//===--- CGContracts.cpp - Emit LLVM Code for C++ contracts -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit contract assertions.
//
//===----------------------------------------------------------------------===//

#include "CGCXXABI.h"
#include "CGCall.h"
#include "CGCleanup.h"
#include "CGDebugInfo.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "ConstantEmitter.h"
#include "ContractProve.h"
#include "TargetInfo.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ScopedPrinter.h"
#include <algorithm>
#include <optional>

using namespace clang;
using namespace CodeGen;

constexpr ContractEvaluationSemantic Enforce =
    ContractEvaluationSemantic::Enforce;
constexpr ContractEvaluationSemantic QuickEnforce =
    ContractEvaluationSemantic::QuickEnforce;
constexpr ContractEvaluationSemantic Observe =
    ContractEvaluationSemantic::Observe;
constexpr ContractEvaluationSemantic Ignore =
    ContractEvaluationSemantic::Ignore;
constexpr ContractEvaluationSemantic NoexceptEnforce =
    ContractEvaluationSemantic::NoexceptEnforce;
constexpr ContractEvaluationSemantic NoexceptObserve =
    ContractEvaluationSemantic::NoexceptObserve;

constexpr ContractDetectionMode PredicateFailed =
    ContractDetectionMode::PredicateFailed;

static llvm::Value *emitContractDataBlock(CodeGenFunction &CGF,
                                          const ContractStmt &S,
                                          llvm::Value *BasicBlock);
static llvm::Function *
getOrCreateDynamicSelector(CodeGenModule &CGM, StringRef Name,
                           ContractDynamicLinkage Linkage, bool ProvideWeak,
                           ContractEvaluationSemantic ConfiguredSem);
constexpr ContractDetectionMode ExceptionRaised =
    ContractDetectionMode::ExceptionRaised;

namespace clang::CodeGen {

enum class ContractEmissionStyle {
  /// Emit the contract violation as an inline basic block immediately following
  /// the predicate. The basic block is not shared by other contracts.
  Inline,

  /// Emit a single shared contract violation handler per-function.
  /// This only works when exceptions are disabled, otherwise the violation
  /// handler
  /// may throw from the violation handler.
  Shared
};

struct CurrentContractInfo {

  const ContractStmt *Contract;
  ContractEmissionStyle Style;
  ContractEvaluationSemantic Semantic;

  llvm::BasicBlock *Violation = nullptr;
  llvm::BasicBlock *End = nullptr;

  llvm::Constant *ViolationInfoGV = nullptr;
};

// A contract enforce block is a block used to create and call the violation
// handler for contracts set to 'enforce'. Such contracts never return after
// reporting a violation.
//
// It is used to create a single block per assertion kind that can be used to
// handle all contract violations of that kind in a function.  The kind is part
// of the __cxa violation entry-point name, so a 'pre'/'post' contract must not
// share a block with an 'assert' contract -- otherwise the enforced violation
// would be reported with the wrong kind.
//
// The block is created lazily, and is only created if a contract of the given
// kind is emitted with an enforce semantic.
struct SharedEnforceBlock {
  static SharedEnforceBlock Create(CodeGenFunction &CGF, ContractKind Kind) {
    SharedEnforceBlock This;

    auto SavedIP = CGF.Builder.saveAndClearIP();

    This.Block = CGF.createBasicBlock("contract.violation.handler");
    CGF.Builder.SetInsertPoint(This.Block);
    This.IncomingPHI = CGF.Builder.CreatePHI(CGF.VoidPtrTy, 4);

    CGF.EmitCxaContractViolationCall(
        Kind, Enforce, PredicateFailed, This.IncomingPHI,
        /*IsNoExcept=*/false, /*IsPostCapture=*/false);
    CGF.Builder.CreateUnreachable();
    CGF.Builder.ClearInsertionPoint();
    CGF.Builder.restoreIP(SavedIP);

    return This;
  }

  llvm::BasicBlock *Block = nullptr;
  llvm::PHINode *IncomingPHI = nullptr;

private:
  SharedEnforceBlock() = default;
};

static void CreateTrap(CodeGenFunction &CGF) {
  auto &Builder = CGF.Builder;
  llvm::CallInst *TrapCall = CGF.EmitTrapCall(llvm::Intrinsic::trap);
  TrapCall->setDoesNotReturn();
  TrapCall->setDoesNotThrow();
  Builder.CreateUnreachable();
  Builder.ClearInsertionPoint();
}

static llvm::BasicBlock *CreateTrapBlock(CodeGenFunction &CGF) {
  auto &Builder = CGF.Builder;
  CGBuilderTy::InsertPoint SavedIP = Builder.saveAndClearIP();
  // Set up the terminate handler.  This block is inserted at the very
  // end of the function by FinishFunction.
  llvm::BasicBlock *ContractViolationTrapBlock =
      CGF.createBasicBlock("contract.violation.trap.handler");
  Builder.SetInsertPoint(ContractViolationTrapBlock);

  CreateTrap(CGF);

  Builder.restoreIP(SavedIP);
  return ContractViolationTrapBlock;
}

struct SharedTrapBlock {
  static SharedTrapBlock Create(CodeGenFunction &CGF) {
    SharedTrapBlock This;
    This.Block = CreateTrapBlock(CGF);
    return This;
  }

  llvm::BasicBlock *Block = nullptr;

private:
  SharedTrapBlock() = default;
};

struct CGContractData {
  // One shared enforce block per kind of written contract (Pre/Post/Assert),
  // indexed by the ContractKind value, so the enforced-violation handler is
  // reported with the contract's real kind rather than a single hardcoded kind.
  // Implicit checks report through their own entry points and have none.
  static constexpr unsigned NumEnforceBlockKinds = 3;
  std::optional<SharedEnforceBlock> EnforceBlocks[NumEnforceBlockKinds];
  std::optional<SharedTrapBlock> TrapBlock;
  std::optional<CurrentContractInfo> CurContract;

  // P3098: per-postcondition i1 flag, true once that postcondition's capture
  // construction has been observed to throw (observe semantic only -- see
  // EmitPostconditionCaptureInit's catch body). Consulted by EmitPostContracts
  // to skip that postcondition's predicate. Only populated for postconditions
  // whose capture initializers can actually throw; absence means "never
  // fails" (EmitPostContracts pull for LoadPostconditionCaptureFailed).
  llvm::DenseMap<const ContractStmt *, Address> CaptureInitFailed;

  // P3595 x P3098: a dynamically selected postcondition with captures chooses
  // its semantic once, where the captures are initialized; this holds the i32
  // effective semantic chosen there (0 for an invalid selection), which the
  // postcondition's check dispatches on instead of calling the selector again.
  llvm::DenseMap<const ContractStmt *, Address> DynSelection;

  SharedTrapBlock &GetSharedTrapBlock(CodeGenFunction &CGF) {
    if (!TrapBlock)
      TrapBlock = SharedTrapBlock::Create(CGF);
    return *TrapBlock;
  }

  SharedEnforceBlock &GetSharedEnforceBlock(CodeGenFunction &CGF,
                                            ContractKind Kind) {
    auto Idx = static_cast<unsigned>(Kind);
    assert(Idx < NumEnforceBlockKinds && "unexpected contract kind");
    if (!EnforceBlocks[Idx])
      EnforceBlocks[Idx] = SharedEnforceBlock::Create(CGF, Kind);
    return *EnforceBlocks[Idx];
  }
};

} // namespace clang::CodeGen

CurrentContractInfo *CodeGenFunction::CurContract() {
  return ContractData->CurContract ? &ContractData->CurContract.value()
                                   : nullptr;
}

namespace {
// Saves and restores the enclosing contract: a contract_assert in a statement
// expression in another contract's predicate is emitted while the outer one
// is current.
struct CurrentContractRAII {
  CurrentContractRAII(CodeGenFunction &CGF, CurrentContractInfo CurContract)
      : CGF(CGF), Outer(std::move(CGF.ContractData->CurContract)) {
    CGF.ContractData->CurContract.emplace(std::move(CurContract));
  }
  ~CurrentContractRAII() {
    assert(CGF.ContractData->CurContract);
    CGF.ContractData->CurContract = std::move(Outer);
  }
  CodeGenFunction &CGF;
  std::optional<CurrentContractInfo> Outer;
};
} // namespace

CGContractData *CGContractDataDeleter::Create() { return new CGContractData(); }
void CGContractDataDeleter::operator()(CGContractData *Data) const {
  delete Data;
}

llvm::BasicBlock *
CodeGenFunction::GetSharedContractViolationEnforceBlock(ContractKind Kind,
                                                        bool Create) {
  auto Idx = static_cast<unsigned>(Kind);
  assert(Idx < CGContractData::NumEnforceBlockKinds &&
         "unexpected contract kind");
  if (!ContractData->EnforceBlocks[Idx] && !Create)
    return nullptr;
  return ContractData->GetSharedEnforceBlock(*this, Kind).Block;
}

llvm::BasicBlock *
CodeGenFunction::GetSharedContractViolationTrapBlock(bool Create) {
  if (!ContractData->TrapBlock && !Create)
    return nullptr;
  return ContractData->GetSharedTrapBlock(*this).Block;
}

// -------------------------------------------------------------------
// New ABI: __cxa_contract_violation_* entry points
// -------------------------------------------------------------------

// Map (kind, semantic, mode) to the entry point name string.
static std::string GetCxaEntryPointName(ContractKind Kind,
                                        ContractEvaluationSemantic Semantic,
                                        ContractDetectionMode Mode,
                                        bool IsPostCapture, bool IsNoExcept) {
  const char *KindStr;
  if (IsPostCapture) {
    KindStr = "post_capture";
  } else {
    switch (Kind) {
    case ContractKind::Pre:
      KindStr = "pre";
      break;
    case ContractKind::Post:
      KindStr = "post";
      break;
    case ContractKind::Assert:
      KindStr = "assert";
      break;
    case ContractKind::Implicit:
      KindStr = "implicit";
      break;
    }
  }

  const char *SemStr;
  switch (Semantic) {
  case Enforce:
    SemStr = "enforce";
    break;
  case Observe:
    SemStr = "observe";
    break;
  case NoexceptEnforce:
    SemStr = "noexcept_enforce";
    break;
  case NoexceptObserve:
    SemStr = "noexcept_observe";
    break;
  default:
    llvm_unreachable("bad semantic for cxa entry point");
  }

  const char *ModeStr = Mode == PredicateFailed ? "pf" : "ex";

  return (llvm::Twine("__cxa_contract_violation_") + KindStr + "_" + SemStr +
          "_" + ModeStr + (IsNoExcept ? "_noexcept" : ""))
      .str();
}

// Get or create the per-TU descriptor table global constant.
// Basic layout (3 entries): source_location, comment, message
// The descriptor table is a packed struct matching __cxa_descriptor_table_t:
//   header (1 byte), num_entries (1 byte), field_ids[N], padding, offsets[N]
static llvm::Constant *getOrCreateDescriptorTable(CodeGenModule &CGM,
                                                  bool HasLocalHandler,
                                                  bool HasQuery = false) {
  const char *Name;
  if (HasLocalHandler && HasQuery)
    Name = "__clang_contract_desc_full";
  else if (HasLocalHandler)
    Name = "__clang_contract_desc_label";
  else if (HasQuery)
    Name = "__clang_contract_desc_query";
  else
    Name = "__clang_contract_desc_basic";

  // The tables are internal, which getGlobalVariable skips unless asked.
  if (auto *Existing =
          CGM.getModule().getGlobalVariable(Name, /*AllowInternal=*/true))
    return Existing;

  llvm::LLVMContext &LLVMCtx = CGM.getLLVMContext();
  llvm::Type *I8Ty = llvm::Type::getInt8Ty(LLVMCtx);
  llvm::Type *PtrSizeTy = CGM.IntPtrTy;

  // Data block layout (basic, 8 fields):
  //   [0] const void* __descriptor_    (ptr size)
  //   [1] const void* __next_          (ptr size)
  //   [2] const char* __file_          (ptr size)
  //   [3] const char* __function_      (ptr size)
  //   [4] unsigned    __line_          (4 bytes)
  //   [5] unsigned    __column_        (4 bytes)
  //   [6] const char* __comment_       (ptr size)
  //   [7] const char* __message_       (ptr size)
  // Extended fields after message (handler+query=full):
  //   [8] void*       __local_handler_ (ptr size)  -- if handler
  //   [8/9] void*     __query_fn_      (ptr size)  -- if query
  //   [9/10] const void* __label_ptr_  (ptr size)  -- if handler or query

  const llvm::DataLayout &DL = CGM.getModule().getDataLayout();
  unsigned PtrSize = DL.getPointerSize();

  uint64_t OffSrcLoc = 2 * PtrSize;
  uint64_t OffComment = 4 * PtrSize + 8;
  uint64_t OffMessage = 5 * PtrSize + 8;

  SmallVector<uint8_t, 6> FieldIDs;
  SmallVector<uint64_t, 6> Offsets;

  FieldIDs.push_back(0x01);
  Offsets.push_back(OffSrcLoc);
  FieldIDs.push_back(0x02);
  Offsets.push_back(OffComment);
  FieldIDs.push_back(0x03);
  Offsets.push_back(OffMessage);

  // Fields 0-3 are PtrSize each. Fields 4-5 (line, column) are 4 bytes
  // each (8 bytes total). Field 6 onward are PtrSize each.
  // offset(field_i) for i >= 6 = (i - 2) * PtrSize + 8
  unsigned NextField = 8;
  if (HasLocalHandler) {
    FieldIDs.push_back(0x04);
    Offsets.push_back((NextField - 2) * PtrSize + 8);
    ++NextField;
  }
  if (HasQuery) {
    FieldIDs.push_back(0x05);
    Offsets.push_back((NextField - 2) * PtrSize + 8);
    ++NextField;
  }
  if (HasLocalHandler || HasQuery) {
    FieldIDs.push_back(0x06);
    Offsets.push_back((NextField - 2) * PtrSize + 8);
  }

  unsigned NumEntries = FieldIDs.size();
  uint8_t Header = (1u << 4) | 0x2;

  unsigned HeaderSize = 2 + NumEntries;
  unsigned AlignTo = PtrSize;
  unsigned Padding = (AlignTo - (HeaderSize % AlignTo)) % AlignTo;

  SmallVector<llvm::Type *, 4> ElemTypes;
  unsigned ByteCount = 2 + NumEntries + Padding;
  ElemTypes.push_back(llvm::ArrayType::get(I8Ty, ByteCount));
  ElemTypes.push_back(llvm::ArrayType::get(PtrSizeTy, NumEntries));

  llvm::StructType *DescTy = llvm::StructType::get(LLVMCtx, ElemTypes,
                                                   /*isPacked=*/true);

  SmallVector<llvm::Constant *, 16> HeaderBytes;
  HeaderBytes.push_back(llvm::ConstantInt::get(I8Ty, Header));
  HeaderBytes.push_back(llvm::ConstantInt::get(I8Ty, NumEntries));
  for (uint8_t FID : FieldIDs)
    HeaderBytes.push_back(llvm::ConstantInt::get(I8Ty, FID));
  for (unsigned i = 0; i < Padding; ++i)
    HeaderBytes.push_back(llvm::ConstantInt::get(I8Ty, 0));

  llvm::Constant *HeaderArr = llvm::ConstantArray::get(
      llvm::ArrayType::get(I8Ty, ByteCount), HeaderBytes);

  SmallVector<llvm::Constant *, 6> OffsetConsts;
  for (uint64_t Off : Offsets)
    OffsetConsts.push_back(llvm::ConstantInt::get(PtrSizeTy, Off));

  llvm::Constant *OffsetArr = llvm::ConstantArray::get(
      llvm::ArrayType::get(PtrSizeTy, NumEntries), OffsetConsts);

  llvm::Constant *Init =
      llvm::ConstantStruct::get(DescTy, {HeaderArr, OffsetArr});

  auto *GV =
      new llvm::GlobalVariable(CGM.getModule(), DescTy, /*isConstant=*/true,
                               llvm::GlobalValue::InternalLinkage, Init, Name);
  GV->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

  return GV;
}

void CodeGenFunction::EmitCxaContractViolationCall(
    ContractKind Kind, ContractEvaluationSemantic Semantic,
    ContractDetectionMode Mode, llvm::Value *DataBlockPtr, bool IsNoExcept,
    bool IsPostCapture) {
  bool IsEnforce = (Semantic == Enforce || Semantic == NoexceptEnforce);

  // The P4298 noexcept semantics in C (an extension): the named
  // _noexcept entry points live in the C++ runtime, so C reports through the
  // C runtime's __dispatch_with_override_core, which takes the kind, the
  // semantic and the detection mode (and aborts after the handler for
  // noexcept_enforce).
  if (!getLangOpts().CPlusPlus &&
      (Semantic == NoexceptEnforce || Semantic == NoexceptObserve)) {
    unsigned KindValue =
        static_cast<unsigned>(getContractAssertionKind(Kind, IsPostCapture));
    llvm::Type *I8 = Builder.getInt8Ty();
    llvm::FunctionType *FTy = llvm::FunctionType::get(
        VoidTy, {Int8PtrTy, I8, I8, I8}, /*isVarArg=*/false);
    llvm::FunctionCallee Fn =
        CGM.CreateRuntimeFunction(FTy, "__dispatch_with_override_core");
    llvm::Value *Args[] = {
        DataBlockPtr, llvm::ConstantInt::get(I8, KindValue),
        llvm::ConstantInt::get(I8, static_cast<unsigned>(Semantic)),
        llvm::ConstantInt::get(I8, Mode == PredicateFailed ? 1 : 2)};
    EmitNounwindRuntimeCall(Fn, Args);
    if (IsEnforce) {
      Builder.CreateUnreachable();
      Builder.ClearInsertionPoint();
    }
    return;
  }

  std::string EntryName =
      GetCxaEntryPointName(Kind, Semantic, Mode, IsPostCapture, IsNoExcept);

  auto &Ctx = getContext();
  CanQualType ArgTypes[1] = {Ctx.VoidPtrTy};

  const CGFunctionInfo &VFuncInfo =
      CGM.getTypes().arrangeBuiltinFunctionDeclaration(getContext().VoidTy,
                                                       ArgTypes);

  llvm::FunctionType *VFTy = CGM.getTypes().GetFunctionType(VFuncInfo);
  llvm::FunctionCallee VFunc = CGM.CreateRuntimeFunction(VFTy, EntryName);

  // Mark enforce entry points as noreturn.
  if (IsEnforce) {
    if (auto *Fn = dyn_cast<llvm::Function>(VFunc.getCallee()))
      Fn->addFnAttr(llvm::Attribute::NoReturn);
  }

  if (IsEnforce) {
    llvm::Value *Args[1] = {DataBlockPtr};
    EmitNoreturnRuntimeCallOrInvoke(VFunc, Args);
    Builder.ClearInsertionPoint();
  } else {
    CallArgList Args;
    Args.add(RValue::get(DataBlockPtr), getContext().VoidPtrTy);
    EmitCall(VFuncInfo, CGCallee::forDirect(VFunc), ReturnValueSlot(), Args);
  }
}

/// The method the local-handler trampoline calls, or null.  The rethrow
/// analysis below has to reason about exactly that method.  Sema synthesized
/// the trampoline (getLabelFacetTrampoline in SemaContract.cpp); its body is
/// the handler call, possibly converted to violation_handled through a
/// conversion function, so look for the call by name.
static const CXXMethodDecl *findLocalViolationHandler(const ContractStmt &S) {
  const FunctionDecl *FD = S.getLocalHandlerFn();
  if (!FD || !FD->getBody())
    return nullptr;
  SmallVector<const Stmt *, 8> Work = {FD->getBody()};
  while (!Work.empty()) {
    const Stmt *St = Work.pop_back_val();
    if (!St)
      continue;
    if (const auto *CE = dyn_cast<CallExpr>(St))
      if (const auto *MD =
              dyn_cast_or_null<CXXMethodDecl>(CE->getDirectCallee()))
        if (MD->getDeclName().isIdentifier() &&
            MD->getName() == "handle_contract_violation")
          return MD;
    for (const Stmt *Child : St->children())
      Work.push_back(Child);
  }
  return nullptr;
}

/// The llvm::Function for a facet trampoline Sema synthesized.  Every contract
/// naming the same label type shares one, and a TU can hold two declarations
/// of it (one read from a module) that are the same function: reuse what the
/// module already holds under the name rather than define it twice.
static llvm::Function *getFacetTrampoline(CodeGenModule &CGM,
                                          const FunctionDecl *FD) {
  if (!FD)
    return nullptr;
  GlobalDecl GD(FD);
  if (llvm::Function *F = CGM.getModule().getFunction(CGM.getMangledName(GD)))
    return F;
  // An internal function is emitted only once it is known to be needed.
  CGM.EmitGlobal(GD);
  return cast<llvm::Function>(CGM.GetAddrOfFunction(GD));
}

// Emit the contract expression.
void CodeGenFunction::EmitContractStmt(const ContractStmt &S) {
  EmitContractStmtAsFullStmt(S);
}

void CodeGenModule::diagnoseUnprovenContract(
    const ContractStmt &S, ContractEvaluationSemantic Sem, const Expr *CallSite,
    const FunctionDecl *Callee, CodeGenFunction *CGF,
    const ContractOverridePair *Override, bool OverrideShadow) {
  assert(isProvingContractSemantic(Sem) && "not prove or diagnose");
  // An invalid assertion has been diagnosed already.
  if (S.isInvalid())
    return;
  // P4405 proving hook.
  ContractProofQuery Query{getContext(), S,   Sem,      S.getCond(),   CallSite,
                           Callee,       CGF, Override, OverrideShadow};
  // An evaluation deferred to a strategy is placed wherever it is emitted.
  if (placeDeferredContractProof(Query, getLangOpts(), /*Defer=*/false) ||
      !DiagnosedUnprovenContracts.insert({&S, CallSite}).second)
    return;
  ContractProofResult Proof = proveContract(Query, getLangOpts());
  if (isContractProofProven(Proof))
    return;
  if (Proof == ContractProofResult::Deferred &&
      placeDeferredContractProof(Query, getLangOpts(), /*Defer=*/true))
    return;
  if (Proof == ContractProofResult::Deferred)
    Proof = ContractProofResult::Unproven;
  if (takeContractDiagnostic(Query, Proof, getLangOpts()))
    return;
  bool AlwaysFalse = Proof == ContractProofResult::AlwaysFalse;
  unsigned ID = Sem == ContractEvaluationSemantic::Prove
                    ? (AlwaysFalse ? diag::err_contract_always_false
                                   : diag::err_contract_unproven)
                    : (AlwaysFalse ? diag::warn_contract_always_false
                                   : diag::warn_contract_unproven);
  SourceRange PredRange = S.getCond()->getSourceRange();
  // In a template instantiation, the notes Sema would give the diagnostic.
  unsigned Context = getContext().getContractDiagnosticContext(
      CGF ? CGF->CurCodeDecl : nullptr);
  // D4405: an override pair's is at D::f's definition (its Site), naming
  // B::f.
  bool IsOverride = Override != nullptr;
  StringRef BaseName = Override ? StringRef(Override->BaseName) : StringRef();
  if (!CallSite) {
    getDiags().Report(S.getKeywordLoc(), ID)
        << PredRange << IsOverride << BaseName;
    getContext().emitContractDiagnosticContext(getDiags(), Context);
    return;
  }
  getDiags().Report(CallSite->getExprLoc(), ID)
      << CallSite->getSourceRange() << IsOverride << BaseName;
  getContext().emitContractDiagnosticContext(getDiags(), Context);
  // The note follows the diagnostic, so it is dropped with a silenced warning.
  getDiags().Report(PredRange.getBegin(),
                    diag::note_contract_unproven_predicate)
      << PredRange;
}

void CodeGenFunction::DiagnoseCallerSideProvingContracts(const FunctionDecl *FD,
                                                         const Expr *CallSite,
                                                         llvm::Value *Object) {
  if (!FD || !getLangOpts().ContractsP4405)
    return;
  // A call of an inheriting constructor calls the inherited one, with the
  // same arguments, whose contracts are the ones evaluated -- for a base
  // subobject, so the object is not that one's `this'.
  while (const auto *CD = dyn_cast<CXXConstructorDecl>(FD)) {
    InheritedConstructor Inh = CD->getInheritedConstructor();
    if (!Inh || Inh.getConstructor() == CD)
      break;
    FD = Inh.getConstructor();
    Object = nullptr;
  }
  const FunctionDecl *WithContracts = nullptr;
  for (const FunctionDecl *R : FD->redecls())
    if (R->hasContracts()) {
      WithContracts = R;
      break;
    }
  if (!WithContracts)
    return;
  llvm::Value *SavedObject = ContractCallObject;
  ContractCallObject = Object;
  const DeclContext *DC = WithContracts->getDeclContext();
  for (const ContractStmt *CS : WithContracts->getContracts()->contracts()) {
    // D4405: the analysis evaluation at a call is a precondition's own --
    // the first evaluation of the assertion, resolved as for the callee's
    // definition, so independent of the caller -- taking the stronger of it
    // and what the caller-side resolution moved into it; a postcondition
    // has only the latter.  In C a precondition is analyzed in the
    // function's body (notadragon_wg21 DECISIONS.md E143 (a)).
    ContractEvaluationSemantic Sem;
    if (CS->getContractKind() == ContractKind::Pre) {
      Sem = CS->ensureCallerAnalysisSemantic(getContext(), DC);
      if (getLangOpts().CPlusPlus)
        Sem = contractAnalysisStronger(
            Sem, CS->ensureAnalysisSemantic(getContext(), DC));
    } else if (CS->getContractKind() == ContractKind::Post) {
      Sem = CS->ensureCallerAnalysisSemantic(getContext(), DC);
    } else {
      continue;
    }
    if (isProvingContractSemantic(Sem))
      CGM.diagnoseUnprovenContract(
          *CS, Sem, CallSite,
          CS->getContractKind() == ContractKind::Pre ? FD : nullptr, this);
  }
  ContractCallObject = SavedObject;
}

const Expr *CodeGenModule::getContractImplicitCallSite(const void *Key,
                                                       SourceLocation Loc) {
  const Expr *&Site = ContractImplicitCallSites[Key];
  if (!Site)
    Site = new (getContext())
        OpaqueValueExpr(Loc, getContext().VoidTy, VK_PRValue);
  return Site;
}

void CodeGenFunction::noteContractDestroySite(Address Addr, const Expr *Site) {
  if (getLangOpts().ContractsP4405 && Addr.isValid())
    ContractDestroySites[Addr.getBasePointer()] = Site;
}

void CodeGenFunction::noteContractDestroySite(Address Addr,
                                              const VarDecl *Var) {
  if (!getLangOpts().ContractsP4405)
    return;
  const auto *RD = Var->getType()->getAsCXXRecordDecl();
  const CXXDestructorDecl *DD = RD ? RD->getDestructor() : nullptr;
  if (!DD || llvm::none_of(DD->redecls(), [](const FunctionDecl *R) {
        return R->hasContracts();
      }))
    return;
  noteContractDestroySite(
      Addr, CGM.getContractImplicitCallSite(Var, Var->getLocation()));
}

void CodeGenFunction::DiagnoseImplicitDtorProvingContracts(
    const CXXDestructorDecl *DD, Address This, const Expr *Site) {
  if (!getLangOpts().ContractsP4405 || !DD)
    return;
  bool HasContracts = false;
  for (const FunctionDecl *R : DD->redecls())
    HasContracts |= R->hasContracts();
  // A destructor variant calling another runs one destructor, not a second.
  if (!HasContracts || (CurGD.getDecl() &&
                        CurGD.getDecl()->getCanonicalDecl() ==
                            DD->getCanonicalDecl()))
    return;
  if (!Site) {
    auto It = ContractDestroySites.find(This.getBasePointer());
    if (It != ContractDestroySites.end())
      Site = It->second;
  }
  if (!Site)
    Site = getContractFallbackDestroySite();
  DiagnoseCallerSideProvingContracts(DD, Site, This.emitRawPointer(*this));
}

const Expr *CodeGenFunction::getContractFallbackDestroySite() {
  // A member or a base destroyed by the function being emitted, an array
  // element, an object whose destruction no expression shows: diagnosed at
  // the end of that function.
  SourceLocation Loc =
      CurCodeDecl ? CurCodeDecl->getLocation() : SourceLocation();
  if (const auto *FD = dyn_cast_or_null<FunctionDecl>(CurCodeDecl))
    if (const Stmt *Body = FD->getBody())
      Loc = Body->getEndLoc();
  return CGM.getContractImplicitCallSite(CurCodeDecl, Loc);
}

void CodeGenFunction::DiagnosePartialAggregateDtorProvingContracts(
    QualType Ty, Address Addr) {
  if (!getLangOpts().ContractsP4405 || !HaveInsertPoint() ||
      Ty.isDestructedType() != QualType::DK_cxx_destructor)
    return;
  const auto *RD = getContext().getBaseElementType(Ty)->getAsCXXRecordDecl();
  if (!RD)
    return;
  // Where the cleanup itself would place it, so that the evaluation is the
  // same one when the cleanup is emitted too: an array is destroyed element
  // by element, at addresses no expression shows.
  DiagnoseImplicitDtorProvingContracts(
      RD->getDestructor(), Addr,
      getContext().getAsArrayType(Ty) ? getContractFallbackDestroySite()
                                      : nullptr);
}

bool CodeGenFunction::isContractAnalyzedHere(const ContractStmt &S) const {
  if (InContractInterfaceEvaluation)
    return false;
  switch (S.getContractKind()) {
  case ContractKind::Pre:
    return !getLangOpts().CPlusPlus;
  case ContractKind::Post:
  case ContractKind::Assert:
    return true;
  default:
    return false;
  }
}

// Build a synthetic try/catch whose sole handler is a catch-all with an empty
// body.  The empty bodies are placeholders: this node is only used to drive the
// EH-scope machinery (EnterCXXTryStmt / ExitCXXTryStmtWithCatchIR).  The
// guarded code and the handler body are both emitted as direct IR by the
// caller, so neither the try body nor the catch body of this node is ever
// emitted via EmitStmt -- in particular, the contract statement is never
// re-emitted.
static CXXTryStmt *BuildContractCatchAllTry(SourceLocation Loc,
                                            CodeGenFunction &CGF) {
  auto &Ctx = CGF.getContext();
  auto *CatchBody =
      CompoundStmt::Create(Ctx, {}, FPOptionsOverride(), Loc, Loc);
  auto *Catch =
      new (Ctx) CXXCatchStmt(Loc, /*exDecl=*/nullptr, /*block=*/CatchBody);
  auto *TryBody = CompoundStmt::Create(Ctx, {}, FPOptionsOverride(), Loc, Loc);
  return CXXTryStmt::Create(Ctx, Loc, TryBody, Catch);
}

// -------------------------------------------------------------------
// P3595 dynamic (link-/run-time) evaluation-semantic selection
// -------------------------------------------------------------------

// Build the violation-info global constant (data block) for a contract, with
// the descriptor-table pointer patched in.  One per data block: the data
// block is uniqued, and every patched copy of it points at the same basic
// descriptor table, so a contract emitted more than once (each arm of a
// dynamic-selection dispatch, say) reuses the first copy.
static llvm::Constant *
FinishViolationInfo(CodeGenFunction &CGF,
                    UnnamedGlobalConstantDecl *ViolationGV) {
  CodeGenModule &CGM = CGF.CGM;
  llvm::Constant *DataGV =
      CGM.GetAddrOfUnnamedGlobalConstantDecl(ViolationGV, "contract.loc")
          .getPointer();

  if (auto *OrigGV = dyn_cast<llvm::GlobalVariable>(DataGV)) {
    if (llvm::GlobalVariable *Patched = CGM.getModule().getGlobalVariable(
            (OrigGV->getName() + ".patched").str(), /*AllowInternal=*/true))
      return Patched;
    llvm::Constant *DescTable =
        getOrCreateDescriptorTable(CGM, /*HasLocalHandler=*/false);
    llvm::Constant *OrigInit = OrigGV->getInitializer();
    if (auto *OrigStruct = dyn_cast<llvm::ConstantStruct>(OrigInit)) {
      SmallVector<llvm::Constant *, 8> NewFields;
      for (unsigned i = 0; i < OrigStruct->getNumOperands(); ++i) {
        if (i == 0)
          NewFields.push_back(DescTable);
        else
          NewFields.push_back(OrigStruct->getOperand(i));
      }
      llvm::Constant *NewInit =
          llvm::ConstantStruct::get(OrigStruct->getType(), NewFields);
      auto *PatchedGV = new llvm::GlobalVariable(
          CGM.getModule(), OrigStruct->getType(),
          /*isConstant=*/true, llvm::GlobalValue::InternalLinkage, NewInit,
          OrigGV->getName() + ".patched");
      PatchedGV->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
      return PatchedGV;
    }
  }
  return DataGV;
}

static llvm::Constant *BuildContractViolationInfo(CodeGenFunction &CGF,
                                                  const ContractStmt &S) {
  return FinishViolationInfo(
      CGF, CGF.getContext().BuildViolationObject(
               &S, dyn_cast_or_null<FunctionDecl>(CGF.CurFuncDecl)));
}

// P3100: resolve the evaluation semantic of an implicit contract assertion for
// the core-language UB named by GROUP, as configured for FNCONTEXT at LOC.  All
// implicit checks share this query: kind Implicit, callee-side, and an allowed
// set of the four C++26 semantics + "assume" ALWAYS (implicit-assume introduces
// no new UB, so it is not gated on -fcontracts-allow-assume) + the P4298
// noexcept variants when -fcontracts-p4298 is in effect -- unless the check
// supplies its own ALLOWEDMASK.  Never the P4405 prove and diagnose: no
// implicit allowed set holds them, so a configuration naming one is adjusted
// to ignore.
static ContractEvaluationSemantic resolveImplicitContractSemantic(
    CodeGenModule &CGM, StringRef Group, const DeclContext *FnContext,
    SourceLocation Loc, std::optional<unsigned> AllowedMask = {}) {
  using CES = ContractEvaluationSemantic;
  std::string GroupStr(Group);
  ContractQuery Q;
  Q.Kind = ContractKind::Implicit;
  Q.CallerSide = false;
  Q.InConstantEvaluation = false;
  Q.Groups = ArrayRef<std::string>(GroupStr);
  Q.FnContext = FnContext;
  Q.Loc = Loc;
  Q.SM = &CGM.getContext().getSourceManager();
  if (AllowedMask) {
    Q.AllowedMask = *AllowedMask;
  } else {
    Q.AllowedMask = AllContractSemanticsMask | (1u << unsigned(CES::Assume));
    if (CGM.getLangOpts().ContractsP4298)
      Q.AllowedMask |= (1u << unsigned(CES::NoexceptEnforce)) |
                       (1u << unsigned(CES::NoexceptObserve));
  }
  assert(!(Q.AllowedMask & ProvingContractSemanticsMask) &&
         "an implicit contract assertion allows prove or diagnose");
  CES Sem = CGM.getLangOpts().ContractOpts.resolveContractSemantic(Q);
  // Every implicit allowed set holds ignore or assume, which the best fit
  // reaches from any semantic.
  assert(Sem != InvalidContractEvaluationSemantic &&
         "no valid semantic for an implicit contract assertion");
  return Sem;
}

// P3100: at the point a value-returning function can fall off its end
// ({stmt.return.flow.off}), resolve the implicit contract assertion's
// evaluation semantic and emit the corresponding reaction.  Returns true when a
// (non-"assume") reaction was emitted -- either a terminating call (the
// insertion point is left cleared) or a defined return branched to the epilogue
// -- in which case the caller must not emit its own missing-return handling.
// Returns false for "assume" (the caller keeps today's UB behaviour).
bool CodeGenFunction::EmitImplicitFlowOffReaction(const FunctionDecl *FD) {
  using CES = ContractEvaluationSemantic;

  CES Sem = resolveImplicitContractSemantic(CGM, "ub:stmt.return.flow.off", FD,
                                            FD->getLocation());

  if (Sem == CES::Assume)
    return false;

  // Store a defined (erroneous) zero into the return slot and branch to the
  // epilogue -- for scalar return types only (P3100 erroneous behaviour is
  // built-in-only).  Guarantees the return value is written, not left
  // indeterminate.
  auto EmitDefinedReturn = [&]() {
    QualType RetTy = FD->getReturnType();
    if (ReturnValue.isValid()) {
      if (hasScalarEvaluationKind(RetTy))
        Builder.CreateStore(llvm::Constant::getNullValue(ConvertType(RetTy)),
                            ReturnValue);
      else {
        // Zero every byte of the result object (including padding) so no
        // indeterminate data leaks, regardless of the return type.
        llvm::Value *Size = llvm::ConstantInt::get(
            CGM.SizeTy, getContext().getTypeSizeInChars(RetTy).getQuantity());
        Builder.CreateMemSet(ReturnValue, Builder.getInt8(0), Size,
                             /*IsVolatile=*/false);
      }
    }
    EmitBranchThroughCleanup(ReturnBlock);
  };

  if (Sem == CES::Ignore) {
    EmitDefinedReturn();
    return true;
  }
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
    return true;
  }

  // enforce / observe / noexcept_enforce / noexcept_observe: report the
  // violation through the CAK_IMPLICIT entry point.
  bool IsNoExcept =
      (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
  bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
  llvm::Constant *Info = FinishViolationInfo(
      *this, getContext().BuildViolationObject(
                 FD->getLocation(),
                 "control reached the end of a value-returning function",
                 std::nullopt, FD));
  EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                               ContractDetectionMode::PredicateFailed, Info,
                               IsNoExcept, /*IsPostCapture=*/false);
  if (!IsEnforce)
    EmitDefinedReturn();
  return true;
}

bool CodeGenFunction::implicitContractApplies(StringRef Group,
                                              SourceLocation Loc) {
  if (!getLangOpts().ContractsP3100)
    return false;
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  return resolveImplicitContractSemantic(CGM, Group, FD, Loc) !=
         ContractEvaluationSemantic::Assume;
}

// P3100: select the __cxa_pure_virtual terminus for a pure virtual
// (ub:class.abstract.pure.virtual).  A call that dispatches to a pure virtual
// is core-language UB, but there is no per-call site (a pure-virtual dispatch
// is an ordinary indirect call, and only the runtime object's current vtable
// knows the slot is pure).  Instead the semantic is resolved here, where the
// vtable is emitted, at MD's declaring (base) class -- so per-file/line and
// per-namespace P3595 config selects the terminus per class.  The vtable slot
// stays a plain function pointer; only its default value changes, from the
// legacy
// __cxa_pure_virtual to a semantic-specific terminus.  Returns an empty
// StringRef for assume/ignore (a pure-virtual call has no defined value to
// substitute) or when -fcontracts-p3100 is off; the caller then keeps
// __cxa_pure_virtual.
//
// The vtable slot is reached only by a dispatched call, so this checks the
// .indirect leaf of the group (the call made through another member
// function); a direct call in the class's own constructor or destructor is
// resolved statically and never reaches the slot (.direct, not checked).
//
// This is a CodeGenModule method (its caller is in CGVTables.cpp), kept here in
// CGContracts.cpp rather than beside the other CodeGenModule members so it can
// share the file-static resolveImplicitContractSemantic with the
// CodeGenFunction EmitImplicit* guards above.
StringRef
CodeGenModule::getPureVirtualContractTerminusName(const CXXMethodDecl *MD) {
  using CES = ContractEvaluationSemantic;
  if (!getLangOpts().ContractsP3100)
    return StringRef();

  const CXXRecordDecl *RD = MD->getParent();
  CES Sem = resolveImplicitContractSemantic(
      *this, "ub:class.abstract.pure.virtual.indirect", RD, RD->getLocation());

  // assume/ignore: keep the legacy terminus (no defined value to substitute).
  if (Sem == CES::Assume || Sem == CES::Ignore)
    return StringRef();

  // A throwing handler must not escape a noexcept pure virtual, so promote a
  // throwing enforce/observe to its terminate-on-throw (noexcept) terminus.
  bool Nothrow = false;
  if (const auto *FPT = MD->getType()->getAs<FunctionProtoType>())
    Nothrow = FPT->isNothrow();
  if (Nothrow) {
    if (Sem == CES::Enforce)
      Sem = CES::NoexceptEnforce;
    else if (Sem == CES::Observe)
      Sem = CES::NoexceptObserve;
  }

  switch (Sem) {
  case CES::QuickEnforce:
    return "__cxa_pure_virtual_quick";
  case CES::Enforce:
    return "__cxa_pure_virtual_enforce";
  case CES::Observe:
    return "__cxa_pure_virtual_observe";
  case CES::NoexceptEnforce:
    return "__cxa_pure_virtual_noexcept_enforce";
  case CES::NoexceptObserve:
    return "__cxa_pure_virtual_noexcept_observe";
  default:
    // assume/ignore were handled above; every other semantic is one of the five
    // termini.
    llvm_unreachable("unexpected contract semantic for pure-virtual terminus");
  }
}

// P3100: control flowing off the end of a coroutine whose promise type has no
// usable return_void ({stmt.return.coroutine.flow.off}) is undefined behavior.
// Emit the configured reaction at the fall-off point.  The point is inside the
// coroutine's try block, so a throwing enforce/observe is caught by
// promise.unhandled_exception().  There is no return value to substitute -- the
// coroutine's return object already exists -- so, unlike
// EmitImplicitFlowOffReaction, the continuing semantics emit no return branch;
// control simply falls through to the final suspend.  assume/ignore emit
// nothing.
void CodeGenFunction::EmitImplicitCoroutineFlowOffReaction(SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:stmt.return.coroutine.flow.off", FD, Loc);

  // assume / ignore: no check; fall through to the final suspend as today.
  if (Sem == CES::Assume || Sem == CES::Ignore)
    return;
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
    return;
  }

  bool IsNoExcept =
      (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
  llvm::Constant *Info = FinishViolationInfo(
      *this,
      getContext().BuildViolationObject(
          Loc, "control flowed off the end of a coroutine", std::nullopt, FD));
  EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                               ContractDetectionMode::PredicateFailed, Info,
                               IsNoExcept, /*IsPostCapture=*/false);
  // enforce/noexcept_enforce: the entry point is noreturn (block terminated).
  // observe/noexcept_observe: the handler returned -- control falls through to
  // the final suspend.
}

llvm::Value *CodeGenFunction::EmitImplicitIntOpGuard(
    QualType Ty, llvm::Value *IsViolation, SourceLocation Loc,
    StringRef GroupName, StringRef Comment,
    llvm::function_ref<llvm::Value *()> EmitOp) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic for the core-language UB named by GroupName (e.g.
  // ub:expr.mul.div.by.zero.int.div, ub:expr.shift.neg.and.width.shl).  See
  // EmitImplicitFlowOffReaction for the allowed-set rationale.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(CGM, GroupName, FD, Loc);

  // assume: today's behaviour -- emit the operation unguarded (UB preserved).
  if (Sem == CES::Assume)
    return EmitOp();

  llvm::Type *ResTy = ConvertType(Ty);
  llvm::BasicBlock *ViolBB = createBasicBlock("ub.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("ub.ok");
  Builder.CreateCondBr(IsViolation, ViolBB, ContBB);

  // Whether the violation path produces a value and continues (ignore /
  // observe / noexcept_observe) versus not returning (enforce / quick_enforce).
  bool ViolContinues = (Sem == CES::Ignore || Sem == CES::Observe ||
                        Sem == CES::NoexceptObserve);
  llvm::BasicBlock *EndBB =
      ViolContinues ? createBasicBlock("ub.end") : nullptr;

  // Violation path: run the reaction *without* executing the UB operation.
  EmitBlock(ViolBB);
  llvm::Value *ViolVal = nullptr;
  llvm::BasicBlock *ViolExit = nullptr;
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
  } else if (Sem == CES::Ignore) {
    ViolVal = llvm::Constant::getNullValue(ResTy); // defined (erroneous) 0
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this,
        getContext().BuildViolationObject(Loc, Comment, std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    if (IsEnforce)
      Builder.CreateUnreachable();
    else
      ViolVal = llvm::Constant::getNullValue(ResTy); // continue with 0
  }
  if (ViolContinues) {
    ViolExit = Builder.GetInsertBlock();
    Builder.CreateBr(EndBB);
  }

  // Continue path: the real operation; the predicate is known false here.
  EmitBlock(ContBB);
  llvm::Value *OpVal = EmitOp();
  if (!ViolContinues)
    return OpVal; // enforce / quick_enforce: no merge, flow continues here

  llvm::BasicBlock *ContExit = Builder.GetInsertBlock();
  Builder.CreateBr(EndBB);
  EmitBlock(EndBB);
  llvm::PHINode *Phi = Builder.CreatePHI(ResTy, 2, "ub.val");
  Phi->addIncoming(ViolVal, ViolExit);
  Phi->addIncoming(OpVal, ContExit);
  return Phi;
}

// Shared reaction tail for an implicit guard that has already branched to
// ViolBB on its violating condition and continues at ContBB (null-dereference,
// misaligned access).  See the declaration in CodeGenFunction.h.
void CodeGenFunction::emitImplicitGuardReaction(ContractEvaluationSemantic Sem,
                                                llvm::BasicBlock *ViolBB,
                                                llvm::BasicBlock *ContBB,
                                                SourceLocation Loc,
                                                StringRef Msg,
                                                const FunctionDecl *FD) {
  using CES = ContractEvaluationSemantic;
  EmitBlock(ViolBB);
  if (Sem == CES::QuickEnforce) {
    // Trap before the guarded access; CreateTrap terminates the block.
    CreateTrap(*this);
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this, getContext().BuildViolationObject(Loc, Msg, std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    // enforce: the entry point is noreturn (block already terminated). observe:
    // the handler returned -- branch to ContBB to continue.
    if (!IsEnforce)
      Builder.CreateBr(ContBB);
  }
  // Continue at the post-guard path; the caller performs the real access here.
  EmitBlock(ContBB);
}

bool CodeGenFunction::EmitImplicitNullDerefGuard(llvm::Value *Ptr,
                                                 SourceLocation Loc,
                                                 bool Access) {
  using CES = ContractEvaluationSemantic;

  // The address of an element checks the pointer it was formed from, and
  // that pointer the one it was formed from in turn, for a chain such as
  // *(p + 1 + 2) (see P3100ElementBase).
  while (true) {
    auto It = P3100ElementBase.find(Ptr);
    if (It == P3100ElementBase.end() || It->second.Element != Ptr ||
        !It->second.Base)
      break;
    Ptr = It->second.Base;
  }

  // Nothing is checked through `this`: a null `this` was reported where the
  // member function was called (DECISIONS.md E100 (a), E106 (a)).  In a
  // lambda, CXXThisValue is the captured `this`.
  if (Ptr == CXXThisValue || Ptr == CXXABIThisValue)
    return false;

  // A check deferred to the call through a pointer to member function whose
  // object expression is being emitted: record it (see P3100DeferNull).
  if (P3100DeferNull && !P3100DeferNull->Ptr) {
    *std::exchange(P3100DeferNull, nullptr) = {Ptr, Loc};
    return true;
  }

  // Resolve the semantic for ub:expr.unary.dereference.nullptr at this site:
  // its .access leaf when a load or store follows the indirection, its
  // .indirection leaf when nothing is accessed.  See
  // EmitImplicitFlowOffReaction for the allowed-set rationale.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM,
      Access ? "ub:expr.unary.dereference.nullptr.access"
             : "ub:expr.unary.dereference.nullptr.indirection",
      FD, Loc);

  // assume / ignore: `*p` is an lvalue with no defined substitute, so leave the
  // raw dereference untouched, as it is without the check.
  if (Sem == CES::Assume || Sem == CES::Ignore)
    return false;

  // Emit `if (Ptr == null) <reaction>` before the real access.  The reaction is
  // a void statement (not a value substitution).
  llvm::BasicBlock *ViolBB = createBasicBlock("nulldref.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("nulldref.ok");
  llvm::Value *IsNull = Builder.CreateIsNull(Ptr);
  Builder.CreateCondBr(IsNull, ViolBB, ContBB);

  // observe reports then proceeds into the real (still-null) dereference.
  emitImplicitGuardReaction(Sem, ViolBB, ContBB, Loc,
                            "null pointer dereference", FD);
  return true;
}

bool CodeGenFunction::isP3100NullCheckMade(SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;
  if (!getLangOpts().ContractsP3100)
    return false;
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  for (StringRef Leaf : {"ub:expr.unary.dereference.nullptr.access",
                         "ub:expr.unary.dereference.nullptr.indirection"}) {
    CES Sem = resolveImplicitContractSemantic(CGM, Leaf, FD, Loc);
    if (Sem != CES::Assume && Sem != CES::Ignore)
      return true;
  }
  return false;
}

void CodeGenFunction::EmitP3100MovedSanitizerNullCheck(
    const P3100DeferredNullCheck &D) {
  if (!D.SanPtr)
    return;
  // The sanitizer's null check alone: every other check of the type check
  // it was moved from was made there, and no P3100 check is wanted here.
  SanitizerSet Skipped;
  Skipped.set(SanitizerKind::All);
  Skipped.set(SanitizerKind::Null, false);
  bool SavedSkipGuard = std::exchange(P3100SkipNullGuard, true);
  EmitTypeCheck(static_cast<TypeCheckKind>(D.SanTCK), D.SanLoc, D.SanPtr,
                D.SanTy, CharUnits::Zero(), Skipped);
  P3100SkipNullGuard = SavedSkipGuard;
}

bool CodeGenFunction::isP3100NonNullPointerOperand(const Expr *E) {
  if (IsWrappedCXXThis(E))
    return true;
  E = E->IgnoreParens();
  while (const auto *CE = dyn_cast<CastExpr>(E)) {
    switch (CE->getCastKind()) {
    case CK_ArrayToPointerDecay:
    case CK_FunctionToPointerDecay:
    case CK_BuiltinFnToFnPtr:
      return true;
    case CK_NoOp:
    case CK_BitCast:
      E = CE->getSubExpr()->IgnoreParens();
      continue;
    default:
      return false;
    }
  }
  const auto *UO = dyn_cast<UnaryOperator>(E);
  return UO && UO->getOpcode() == UO_AddrOf;
}

bool CodeGenFunction::isP3100IndirectionGLValue(const Expr *E,
                                                SourceLocation &Loc) {
  E = E->IgnoreParens();
  if (const auto *CE = dyn_cast<CastExpr>(E)) {
    if (CE->getCastKind() != CK_NoOp && CE->getCastKind() != CK_LValueBitCast)
      return false;
    return isP3100IndirectionGLValue(CE->getSubExpr(), Loc);
  }
  if (const auto *UO = dyn_cast<UnaryOperator>(E)) {
    if (UO->getOpcode() == UO_Extension)
      return isP3100IndirectionGLValue(UO->getSubExpr(), Loc);
    if (UO->getOpcode() != UO_Deref ||
        isP3100NonNullPointerOperand(UO->getSubExpr()))
      return false;
    Loc = UO->getExprLoc();
    return true;
  }
  // The address of the result is the address of the operand chosen.
  if (const auto *CO = dyn_cast<ConditionalOperator>(E))
    return isP3100IndirectionGLValue(CO->getTrueExpr(), Loc) ||
           isP3100IndirectionGLValue(CO->getFalseExpr(), Loc);
  if (const auto *BO = dyn_cast<BinaryOperator>(E);
      BO && BO->getOpcode() == BO_Comma)
    return isP3100IndirectionGLValue(BO->getRHS(), Loc);
  return false;
}

void CodeGenFunction::EmitImplicitIndirectionGuard(llvm::Value *Ptr,
                                                   SourceLocation Loc) {
  // The address of a local or a global is not null.
  if (isa<llvm::AllocaInst, llvm::GlobalValue>(Ptr->stripPointerCasts()))
    return;
  EmitImplicitNullDerefGuard(Ptr, Loc, /*Access=*/false);
}

void CodeGenFunction::EmitImplicitFunctionIndirectionGuard(const Expr *E,
                                                           llvm::Value *Ptr) {
  if (!getLangOpts().ContractsP3100)
    return;
  // `*fp` with fp a null pointer to function is an indirection through a null
  // pointer ([expr.unary.op]) whether the function is then called or a
  // reference is bound to it.  The glvalue's address is fp's value (for a
  // conditional, the value of the pointer chosen).
  SourceLocation Loc;
  if (isP3100IndirectionGLValue(E, Loc))
    EmitImplicitIndirectionGuard(Ptr, Loc);
}

bool CodeGenFunction::EmitImplicitMisalignedGuard(llvm::Value *Ptr,
                                                  llvm::Align Align,
                                                  SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic for ub:basic.align.object.alignment at this site.
  // Every access-site check is the group's .store leaf: the check is at a
  // typed access, which a creation performing no store (.nostore) never
  // makes.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:basic.align.object.alignment.store", FD, Loc);

  // assume / ignore: a misaligned access is an lvalue with no defined
  // substitute, so leave the raw access untouched, as it is without the check.
  if (Sem == CES::Assume || Sem == CES::Ignore)
    return false;

  // Emit `if ((Ptr & (Align - 1)) != 0) <reaction>` before the real access.
  llvm::Value *PtrInt = Builder.CreatePtrToInt(Ptr, IntPtrTy);
  llvm::Value *Masked = Builder.CreateAnd(
      PtrInt, llvm::ConstantInt::get(IntPtrTy, Align.value() - 1));
  llvm::Value *IsMisaligned = Builder.CreateIsNotNull(Masked);
  llvm::BasicBlock *ViolBB = createBasicBlock("misalign.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("misalign.ok");
  Builder.CreateCondBr(IsMisaligned, ViolBB, ContBB);

  // observe reports then proceeds into the real (still-misaligned) access.
  emitImplicitGuardReaction(Sem, ViolBB, ContBB, Loc,
                            "misaligned pointer access", FD);
  return true;
}

void CodeGenFunction::EmitCXXAssumeCheck(const Expr *Cond,
                                         ContractEvaluationSemantic Sem,
                                         SourceLocation Loc,
                                         const FunctionDecl *FD) {
  using CES = ContractEvaluationSemantic;

  // Evaluate the (side-effect-free) predicate and branch: true -> continue,
  // false -> the violation reaction.
  llvm::Value *CondVal = EvaluateExprAsBool(Cond);
  llvm::BasicBlock *ViolBB = createBasicBlock("assume.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("assume.ok");
  Builder.CreateCondBr(CondVal, ContBB, ViolBB);

  EmitBlock(ViolBB);
  bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce ||
                    Sem == CES::QuickEnforce);
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    llvm::Constant *Info = FinishViolationInfo(
        *this, getContext().BuildViolationObject(
                   Loc, "assumed condition is false", std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    // enforce/noexcept_enforce: the entry point is noreturn (block already
    // terminated).  observe/noexcept_observe: the handler returned -- continue
    // (with the predicate possibly false, so no assume hint below).
    if (Sem == CES::Observe || Sem == CES::NoexceptObserve)
      Builder.CreateBr(ContBB);
  }

  EmitBlock(ContBB);
  // Enforcing family: the predicate provably holds here, so keep the optimizer
  // hint.  The observing family must not (execution may continue with it
  // false).
  if (IsEnforce)
    Builder.CreateAssumption(CondVal);
}

/// P3100: whether the [[assume]] predicate S has a side effect written in it
/// -- an assignment, an increment or decrement, or a volatile access -- as
/// opposed to one it might have only through a call, which is opaque here.
/// Unevaluated operands and lambda bodies are not part of its evaluation.
static bool assumeHasOwnSideEffect(const Stmt *S) {
  if (const auto *BO = dyn_cast<BinaryOperator>(S)) {
    if (BO->isAssignmentOp())
      return true;
  } else if (const auto *UO = dyn_cast<UnaryOperator>(S)) {
    if (UO->isIncrementDecrementOp())
      return true;
  } else if (const auto *CE = dyn_cast<CastExpr>(S)) {
    if (CE->getCastKind() == CK_LValueToRValue &&
        CE->getSubExpr()->getType().isVolatileQualified())
      return true;
  } else if (isa<UnaryExprOrTypeTraitExpr, CXXNoexceptExpr, LambdaExpr>(S)) {
    return false;
  } else if (const auto *TE = dyn_cast<CXXTypeidExpr>(S)) {
    if (!TE->isPotentiallyEvaluated())
      return false;
  }
  for (const Stmt *Child : S->children())
    if (Child && assumeHasOwnSideEffect(Child))
      return true;
  return false;
}

void CodeGenFunction::EmitCXXAssumeAttr(const CXXAssumeAttr *AA) {
  using CES = ContractEvaluationSemantic;

  const Expr *Assumption = AA->getAssumption();
  if (!getLangOpts().CXXAssumptions || !Builder.GetInsertBlock())
    return;

  bool HasSideEffects = Assumption->HasSideEffects(getContext());

  // P3100: [[assume]] is a configurable implicit contract assertion, group
  // ub:dcl.attr.assume.false (the assumed condition being false is that UB).
  // The predicate's properties set the allowed set for this instance: a
  // side-effect-free predicate is safe to evaluate, so it supports the full
  // checking set; an opaque/side-effecting predicate cannot be evaluated, so
  // only assume/ignore apply (a configured checking semantic clamps to ignore).
  if (getLangOpts().ContractsP3100) {
    const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
    // A checkable (side-effect-free) predicate is a distinct,
    // separately-matchable subset of this check -- same pattern as e.g.
    // ub:expr.unary.dereference.nullptr -- so it gets its own qualified group
    // id.  An uncheckable one reports .nonpure.side.effects when it has a side
    // effect of its own and .nonpure.opaque when it might have one only
    // through a call; the bare .nonpure is never reported.
    std::optional<unsigned> Mask;
    if (HasSideEffects)
      Mask = (1u << unsigned(CES::Ignore)) | (1u << unsigned(CES::Assume));
    StringRef Group = "ub:dcl.attr.assume.false.pure";
    if (HasSideEffects)
      Group = assumeHasOwnSideEffect(Assumption)
                  ? "ub:dcl.attr.assume.false.nonpure.side.effects"
                  : "ub:dcl.attr.assume.false.nonpure.opaque";
    CES Sem = resolveImplicitContractSemantic(CGM, Group, FD,
                                              Assumption->getExprLoc(), Mask);

    if (Sem == CES::Ignore)
      return;
    if (Sem != CES::Assume) {
      EmitCXXAssumeCheck(Assumption, Sem, Assumption->getExprLoc(), FD);
      return;
    }
    // assume: fall through to the status-quo llvm.assume.
  }

  // assume (status quo): only emit the hint when the predicate is side-effect-
  // free (evaluating it for llvm.assume must be unobservable).
  if (!HasSideEffects) {
    llvm::Value *AssumptionVal = EmitCheckedArgForAssume(Assumption);
    Builder.CreateAssumption(AssumptionVal);
  }
}

llvm::Value *CodeGenFunction::EmitImplicitSignedOverflowOp(
    QualType Ty, SourceLocation Loc, StringRef GroupName, StringRef Comment,
    ImplicitOverflowOp Op, llvm::Value *LHS, llvm::Value *RHS) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic for GroupName, the leaf of
  // ub:expr.expr.eval.signed.integer for this operation, at this site.
  // See EmitImplicitFlowOffReaction for the allowed-set rationale; Clang's
  // codegen is always in a valid EH context, so throwing enforce/observe are
  // supported here (no need to exclude them as the GCC middle-end does).
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(CGM, GroupName, FD, Loc);

  auto EmitNSW = [&]() -> llvm::Value * {
    switch (Op) {
    case ImplicitOverflowOp::Add:
      return Builder.CreateNSWAdd(LHS, RHS, "add");
    case ImplicitOverflowOp::Sub:
      return Builder.CreateNSWSub(LHS, RHS, "sub");
    case ImplicitOverflowOp::Mul:
      return Builder.CreateNSWMul(LHS, RHS, "mul");
    }
    llvm_unreachable("bad ImplicitOverflowOp");
  };
  auto EmitPlain = [&]() -> llvm::Value * {
    switch (Op) {
    case ImplicitOverflowOp::Add:
      return Builder.CreateAdd(LHS, RHS, "add");
    case ImplicitOverflowOp::Sub:
      return Builder.CreateSub(LHS, RHS, "sub");
    case ImplicitOverflowOp::Mul:
      return Builder.CreateMul(LHS, RHS, "mul");
    }
    llvm_unreachable("bad ImplicitOverflowOp");
  };

  // assume: keep the no-overflow assumption (nsw), as without the check.
  if (Sem == CES::Assume)
    return EmitNSW();
  // ignore: defined 2's-complement wrapping, no check (-fwrapv-equivalent).
  if (Sem == CES::Ignore)
    return EmitPlain();

  // Checking semantic: compute the wrapped result and the overflow bit
  // together, then branch on overflow to the reaction.  The wrapped result
  // dominates the continuation, so observe/noexcept_observe continue with it
  // and no PHI is needed.
  llvm::Intrinsic::ID IID;
  switch (Op) {
  case ImplicitOverflowOp::Add:
    IID = llvm::Intrinsic::sadd_with_overflow;
    break;
  case ImplicitOverflowOp::Sub:
    IID = llvm::Intrinsic::ssub_with_overflow;
    break;
  case ImplicitOverflowOp::Mul:
    IID = llvm::Intrinsic::smul_with_overflow;
    break;
  }
  llvm::Function *Intrin = CGM.getIntrinsic(IID, LHS->getType());
  llvm::Value *Pair = Builder.CreateCall(Intrin, {LHS, RHS});
  llvm::Value *Result = Builder.CreateExtractValue(Pair, 0, "ovf.res");
  llvm::Value *Overflowed = Builder.CreateExtractValue(Pair, 1, "ovf.bit");

  llvm::BasicBlock *ViolBB = createBasicBlock("ovf.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("ovf.ok");
  Builder.CreateCondBr(Overflowed, ViolBB, ContBB);

  EmitBlock(ViolBB);
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this,
        getContext().BuildViolationObject(Loc, Comment, std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    // enforce/noexcept_enforce: the entry point is noreturn and already
    // terminated the block.  observe/noexcept_observe: the handler returned --
    // continue with the defined wrapped result (report then proceed).
    if (IsEnforce)
      Builder.CreateUnreachable();
    else
      Builder.CreateBr(ContBB);
  }

  EmitBlock(ContBB);
  return Result;
}

llvm::Value *
CodeGenFunction::EmitImplicitInvalidValueGuard(llvm::Value *Loaded, QualType Ty,
                                               SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  if (!getLangOpts().ContractsP3100)
    return Loaded;

  // Only bool and (non-fixed C++) enum loads can carry an invalid value; mirror
  // the -fsanitize=bool/enum detection in EmitScalarRangeCheck.  The contract
  // takes precedence over that sanitizer: the caller runs the sanitizer check
  // (via maybeAttachRangeForLoad) only when this returns the load unchanged,
  // i.e. when no contract applies.
  bool IsBool = Ty->hasBooleanRepresentation() && !Ty->isVectorType();
  bool IsEnum = Ty->isEnumeralType();
  if (!IsBool && !IsEnum)
    return Loaded;
  // A single-bit bool cannot be out of range.
  if (IsBool && cast<llvm::IntegerType>(Loaded->getType())->getBitWidth() == 1)
    return Loaded;

  // Valid range [Min, End) for the loaded storage value.  getStrictEnumRange
  // also filters out enums the sanitizer ignores (e.g. a fixed underlying type
  // covering all bit patterns), which cannot be out of range.
  llvm::APInt Min, End;
  if (IsBool) {
    unsigned Bits = getContext().getTypeSize(Ty);
    Min = llvm::APInt(Bits, 0);
    End = llvm::APInt(Bits, 2);
  } else if (!getStrictEnumRange(Ty, Min, End))
    return Loaded;

  // Resolve the semantic once for this site; assume leaves the raw load (so the
  // caller attaches range metadata, i.e. the optimizer may assume validity).
  // A bool load reports the .bool leaf and an enumeration load .enum.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM,
      IsBool ? "ub:conv.lval.valid.representation.bool"
             : "ub:conv.lval.valid.representation.enum",
      FD, Loc);
  if (Sem == CES::Assume)
    return Loaded;

  // in_range predicate over the storage value, exactly as EmitScalarRangeCheck.
  auto &Ctx = getLLVMContext();
  --End;
  llvm::Value *InRange;
  if (!Min)
    InRange = Builder.CreateICmpULE(Loaded, llvm::ConstantInt::get(Ctx, End));
  else {
    llvm::Value *Upper =
        Builder.CreateICmpSLE(Loaded, llvm::ConstantInt::get(Ctx, End));
    llvm::Value *Lower =
        Builder.CreateICmpSGE(Loaded, llvm::ConstantInt::get(Ctx, Min));
    InRange = Builder.CreateAnd(Upper, Lower);
  }
  llvm::Value *IsInvalid = Builder.CreateNot(InRange, "invalid");

  // The defined valid value substituted for an out-of-range load is 0 (false
  // for bool, an in-range value for enum); the specification permits any valid
  // value.  Compute value = IsInvalid ? 0 : Loaded unconditionally (it
  // dominates the continuation, so no PHI is needed); then, for a checking
  // semantic, add a branch on IsInvalid whose only job is the reaction's side
  // effect.
  llvm::Value *Zero = llvm::Constant::getNullValue(Loaded->getType());
  llvm::Value *Result = Builder.CreateSelect(IsInvalid, Zero, Loaded, "ok.val");
  if (Sem == CES::Ignore)
    return Result;

  llvm::BasicBlock *ViolBB = createBasicBlock("invalid.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("invalid.ok");
  Builder.CreateCondBr(IsInvalid, ViolBB, ContBB);

  EmitBlock(ViolBB);
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this, getContext().BuildViolationObject(
                   Loc, "invalid value for its type", std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    if (IsEnforce)
      Builder.CreateUnreachable();
    else
      Builder.CreateBr(ContBB);
  }

  EmitBlock(ContBB);
  return Result;
}

ContractEvaluationSemantic
CodeGenFunction::resolveImplicitSemantic(StringRef Group, SourceLocation Loc) {
  if (!getLangOpts().ContractsP3100)
    return ContractEvaluationSemantic::Assume;
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  return resolveImplicitContractSemantic(CGM, Group, FD, Loc);
}

llvm::Value *CodeGenFunction::EmitImplicitSubstitutingGuard(
    ContractEvaluationSemantic Sem, llvm::Value *IsViolation,
    llvm::Value *Value, llvm::Value *Substitute, SourceLocation Loc,
    StringRef Comment) {
  using CES = ContractEvaluationSemantic;
  if (Sem == CES::Assume)
    return Value;
  // A condition that folded to false (a constant position in range) can
  // never be a violation: emit no guard, as the checks that test a constant
  // position first do.
  if (const auto *C = dyn_cast<llvm::ConstantInt>(IsViolation);
      C && C->isZero())
    return Value;

  // value = IsViolation ? Substitute : Value, computed unconditionally (it
  // dominates the continuation, so no PHI is needed).
  llvm::Value *Result =
      Builder.CreateSelect(IsViolation, Substitute, Value, "ub.subst");
  if (Sem == CES::Ignore)
    return Result;

  llvm::BasicBlock *ViolBB = createBasicBlock("ub.subst.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("ub.subst.ok");
  Builder.CreateCondBr(IsViolation, ViolBB, ContBB);

  EmitBlock(ViolBB);
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
  } else {
    const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this,
        getContext().BuildViolationObject(Loc, Comment, std::nullopt, FD));
    EmitCxaContractViolationCall(ContractKind::Implicit, Sem,
                                 ContractDetectionMode::PredicateFailed, Info,
                                 IsNoExcept, /*IsPostCapture=*/false);
    if (IsEnforce)
      Builder.CreateUnreachable();
    else
      Builder.CreateBr(ContBB);
  }

  EmitBlock(ContBB);
  return Result;
}

llvm::Value *CodeGenFunction::EmitImplicitArrayBoundsGuard(
    llvm::Value *Idx, bool IdxSigned, llvm::Value *Bound, bool AllowEnd,
    StringRef Group, StringRef Comment, SourceLocation Loc) {
  // Resolve the semantic once for this index; assume leaves the raw index
  // and emits no predicate.
  ContractEvaluationSemantic Sem = resolveImplicitSemantic(Group, Loc);
  if (Sem == ContractEvaluationSemantic::Assume)
    return Idx;

  // out_of_range predicate, mirroring EmitBoundsCheckImpl: widen the index and
  // the (non-negative constant) bound to a common type and compare unsigned so
  // a negative index is caught too.  For an element the valid range is
  // [0,bound); where the one-past index is valid (AllowEnd), index == bound is
  // allowed.
  unsigned IdxBits = cast<llvm::IntegerType>(Idx->getType())->getBitWidth();
  unsigned BoundBits = cast<llvm::IntegerType>(Bound->getType())->getBitWidth();
  llvm::Type *Ty = IdxBits >= BoundBits ? Idx->getType() : Bound->getType();
  llvm::Value *IdxW = Builder.CreateIntCast(Idx, Ty, IdxSigned);
  llvm::Value *BoundW = Builder.CreateIntCast(Bound, Ty, /*isSigned=*/false);
  llvm::Value *IsViolation = AllowEnd
                                 ? Builder.CreateICmpUGT(IdxW, BoundW, "oob")
                                 : Builder.CreateICmpUGE(IdxW, BoundW, "oob");

  // Redirect an out-of-range index to the valid index 0.
  return EmitImplicitSubstitutingGuard(
      Sem, IsViolation, Idx, llvm::Constant::getNullValue(Idx->getType()), Loc,
      Comment);
}

// P3100: arithmetic or a subscript applied to known-bound pointer arithmetic,
// (a + i) + j or (a + i)[j], is checked against a's bound at the position
// i + j.  EmitPointerArithmetic records each known-bound result it produces
// in P3100KnownArith; the result value must match, so a record left by an
// earlier emission of the same expression is never used.
const CodeGenFunction::P3100KnownArithInfo *
CodeGenFunction::getP3100KnownArith(const Expr *E, llvm::Value *Ptr) {
  auto It = P3100KnownArith.find(E->IgnoreParens());
  if (It == P3100KnownArith.end() || It->second.Result != Ptr)
    return nullptr;
  return &It->second;
}

llvm::Value *CodeGenFunction::EmitP3100NestedPosition(llvm::Value *Pos,
                                                      llvm::Value *Idx,
                                                      bool IdxSigned,
                                                      llvm::ConstantInt *Bound) {
  unsigned PosBits = cast<llvm::IntegerType>(Pos->getType())->getBitWidth();
  unsigned IdxBits = cast<llvm::IntegerType>(Idx->getType())->getBitWidth();
  llvm::Type *Ty = PosBits >= IdxBits ? Pos->getType() : Idx->getType();
  llvm::Value *PosW = Builder.CreateIntCast(Pos, Ty, /*isSigned=*/true);
  llvm::Value *IdxW = Builder.CreateIntCast(Idx, Ty, IdxSigned);
  llvm::Value *Sum = Builder.CreateAdd(PosW, IdxW, "known.pos");
  if (IdxSigned)
    return Sum;
  // An unsigned index above the bound is outside the array wherever it is
  // applied; the sum could wrap back into range.
  llvm::Value *BoundW = Builder.CreateIntCast(Bound, Ty, /*isSigned=*/false);
  return Builder.CreateSelect(
      Builder.CreateICmpUGT(IdxW, BoundW),
      Builder.CreateAdd(BoundW, llvm::ConstantInt::get(Ty, 1)), Sum);
}

// P3100: applying a null pointer to member with .* or ->* is undefined
// ({expr.mptr.oper.member.func.null}).  The ABI's null test is the
// representation's: a data member offset of -1, a zero function pointer (with
// a clear virtual bit where the ABI keeps it in the adjustment).  Nothing
// defined can replace the member pointer, so observe continues with it.
void CodeGenFunction::EmitImplicitNullMemberPointerGuard(
    llvm::Value *MemPtr, const MemberPointerType *MPT, SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;
  if (!getLangOpts().ContractsP3100 || !HaveInsertPoint() ||
      isa<llvm::Constant>(MemPtr))
    return;
  bool Function = MPT->isMemberFunctionPointer();
  CES Sem = resolveImplicitSemantic(
      Function ? "ub:expr.mptr.oper.member.func.null.function"
               : "ub:expr.mptr.oper.member.func.null.data",
      Loc);
  if (Sem == CES::Assume || Sem == CES::Ignore)
    return;
  llvm::Value *IsNotNull =
      CGM.getCXXABI().EmitMemberPointerIsNotNull(*this, MemPtr, MPT);
  llvm::BasicBlock *ViolBB = createBasicBlock("mptr.null.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("mptr.null.ok");
  Builder.CreateCondBr(IsNotNull, ContBB, ViolBB);
  emitImplicitGuardReaction(Sem, ViolBB, ContBB, Loc,
                            Function ? "null pointer to member function"
                                     : "null pointer to data member",
                            dyn_cast_or_null<FunctionDecl>(CurFuncDecl));
}

// P3098: emit a postcondition's capture-construction DeclStmt, converting a
// construction exception into a post_capture violation. See the
// CodeGenFunction.h declaration for the contract.
void CodeGenFunction::EmitPostconditionCaptureInit(
    const ContractStmt *CS, ContractEvaluationSemantic Sem,
    llvm::Value *DynSel) {
  const DeclStmt *CapturesStmt = CS->getCapturesDeclStmt();
  const bool GuardThrows =
      getLangOpts().Exceptions && getLangOpts().ContractExceptions;

  // Unlike the predicate-throw path (emitCheckForSemantic), which skips the
  // try/catch when Sema found the predicate cannot throw
  // (ContractStmt::predicateCanThrow), always wrap capture construction here:
  // nothing records whether a capture's construction can throw, and missing
  // one would let the exception propagate unguarded.
  Address FailedFlag = Address::invalid();
  llvm::Constant *ViolationInfo = nullptr;
  CXXTryStmt *Try = nullptr;
  if (GuardThrows) {
    FailedFlag = CreateTempAlloca(Builder.getInt1Ty(), CharUnits::One(),
                                  "contract.capture.failed");
    Builder.CreateStore(Builder.getFalse(), FailedFlag);
    ContractData->CaptureInitFailed.insert({CS, FailedFlag});
    ViolationInfo = BuildContractViolationInfo(*this, *CS);
    Try = BuildContractCatchAllTry(CS->getCapturesDeclStmt()->getBeginLoc(),
                                   *this);
  }

  // Catch body for a capture's construction try/catch: report the exception as
  // a post_capture violation.  Unlike the predicate-throw path there is no
  // predicate to re-check afterward -- construction either succeeded (no catch)
  // or failed, and 'observe' must skip the predicate entirely (recorded via
  // FailedFlag) rather than fall through to evaluate it.  For a dynamically
  // selected postcondition the semantic is the one chosen into DynSel.
  auto ReportWith = [&](ContractEvaluationSemantic S) {
    if (S == Enforce || S == Observe || S == NoexceptEnforce ||
        S == NoexceptObserve) {
      EmitCxaContractViolationCall(
          ContractKind::Post, S, ExceptionRaised, ViolationInfo,
          /*IsNoExcept=*/S == NoexceptEnforce || S == NoexceptObserve,
          /*IsPostCapture=*/true);
      if (HaveInsertPoint() && (S == Observe || S == NoexceptObserve))
        Builder.CreateStore(Builder.getTrue(), FailedFlag);
    } else if (S == QuickEnforce) {
      CreateTrap(*this);
    } else {
      llvm_unreachable("Unhandled semantic");
    }
  };
  auto EmitCaptureCatchBody = [&] {
    if (!DynSel) {
      ReportWith(Sem);
      return;
    }
    // Only a semantic that evaluates gets here: the captures are not
    // initialized otherwise.
    llvm::BasicBlock *End = createBasicBlock("contract.capture.catch.end");
    llvm::SwitchInst *Switch = Builder.CreateSwitch(DynSel, End);
    for (ContractEvaluationSemantic S :
         {Observe, Enforce, QuickEnforce, NoexceptObserve, NoexceptEnforce}) {
      llvm::BasicBlock *Arm = createBasicBlock("contract.capture.catch.arm");
      Switch->addCase(Builder.getInt32(static_cast<unsigned>(S)), Arm);
      EmitBlock(Arm);
      ReportWith(S);
      if (HaveInsertPoint())
        Builder.CreateBr(End);
    }
    EmitBlock(End);
  };

  // Construct each capture in its own try/catch region, split into the
  // Alloca/Init/Cleanups steps ExitCXXTryStmt normally performs together via
  // EmitAutoVarDecl. This matters because the catch teardown requires the catch
  // scope it pushed to still be the top of the EHScopeStack -- but a
  // capture's destructor cleanup must persist past this function (the
  // destruction order and the body-throws path both depend on it living until
  // the real function exit), so it cannot be pushed until after the catch
  // scope is torn down. Deferring EmitAutoVarCleanups until after the catch
  // teardown satisfies both constraints; a still-throwing later
  // capture in the same DeclStmt correctly unwinds this one via that
  // (by-then-active) cleanup, exactly like ordinary sequential construction.
  //
  // Each capture's destructor cleanup is conditional on a flag set once that
  // capture is constructed.  The cleanups are lexical, so they are active on
  // every path out of here, including those on which a capture was never
  // constructed: an observe-reported throw, which skips the captures after
  // it, and a dynamic selection that does not evaluate the postcondition,
  // which skips them all.
  SmallVector<AutoVarEmission, 2> Emissions;
  SmallVector<RawAddress, 2> LiveFlags;
  for (Decl *D : CapturesStmt->decls()) {
    Emissions.push_back(EmitAutoVarAlloca(*cast<VarDecl>(D)));
    RawAddress Live = CreateTempAlloca(Builder.getInt1Ty(), CharUnits::One(),
                                       "contract.capture.live");
    EnsureInsertPoint();
    Builder.CreateStore(Builder.getFalse(), Live);
    LiveFlags.push_back(Live);
  }

  llvm::BasicBlock *DoneBB = createBasicBlock("contract.capture.done");
  if (DynSel) {
    // Ignore and assume have no effect, and an invalid selection is reported
    // by the postcondition's check without reading a capture: initialize the
    // captures only for a semantic that evaluates the postcondition.
    llvm::Value *Evaluates = Builder.getFalse();
    for (ContractEvaluationSemantic S :
         {Observe, Enforce, QuickEnforce, NoexceptObserve, NoexceptEnforce})
      Evaluates = Builder.CreateOr(
          Evaluates,
          Builder.CreateICmpEQ(DynSel,
                               Builder.getInt32(static_cast<unsigned>(S))));
    llvm::BasicBlock *InitBB = createBasicBlock("contract.capture.init");
    Builder.CreateCondBr(Evaluates, InitBB, DoneBB);
    EmitBlock(InitBB);
  }

  for (unsigned I = 0, E = Emissions.size(); I != E; ++I) {
    EnsureInsertPoint();
    if (GuardThrows) {
      EnterCXXTryStmt(*Try);
      EmitAutoVarInit(Emissions[I]);
      ExitCXXTryStmtWithCatchIR(*Try, EmitCaptureCatchBody);

      llvm::Value *Failed = Builder.CreateLoad(FailedFlag);
      llvm::BasicBlock *ContBB = createBasicBlock("contract.capture.cont");
      Builder.CreateCondBr(Failed, DoneBB, ContBB);
      EmitBlock(ContBB);
    } else {
      EmitAutoVarInit(Emissions[I]);
    }
    Builder.CreateStore(Builder.getTrue(), LiveFlags[I]);

    EHScopeStack::stable_iterator OldTop = EHStack.stable_begin();
    EmitAutoVarCleanups(Emissions[I]);
    EHScopeStack::stable_iterator Top = EHStack.stable_begin();
    for (auto It = EHStack.find(Top), End = EHStack.find(OldTop); It != End;
         ++It)
      if (auto *Cleanup = dyn_cast<EHCleanupScope>(&*It)) {
        Cleanup->setActiveFlag(LiveFlags[I]);
        Cleanup->setTestFlagInNormalCleanup();
        Cleanup->setTestFlagInEHCleanup();
      }
  }
  EnsureInsertPoint();
  Builder.CreateBr(DoneBB);
  EmitBlock(DoneBB);
}

llvm::Value *
CodeGenFunction::EmitDynamicEffectiveSelection(const ContractStmt &S) {
  llvm::Function *Selector = getOrCreateDynamicSelector(
      CGM, S.getDynName(), S.getDynLinkage(), S.getDynProvideWeak(),
      S.getDynConfiguredSemantic());
  EnsureInsertPoint();
  // An ordinary call: an exception from the selector unwinds through this
  // function's cleanups and handlers like any other.
  llvm::Value *Raw = EmitCallOrInvoke(Selector, {}, "contract.dyn.raw");
  llvm::Value *RawExt = Builder.CreateZExt(Raw, Int32Ty, "contract.dyn.sem");
  // Map each raw value to its effective semantic (getDynTransform); any other
  // value, and a disallowed transform, is 0, the invalid selection.
  llvm::Value *Eff = Builder.getInt32(0);
  for (unsigned R = 1; R <= ContractStmt::NumDynRawSemantics; ++R)
    if (unsigned E = static_cast<unsigned>(S.getDynTransform(R)))
      Eff = Builder.CreateSelect(
          Builder.CreateICmpEQ(RawExt, Builder.getInt32(R)),
          Builder.getInt32(E), Eff);
  return Eff;
}

bool CodeGenFunction::EmitPostconditionCaptures(const ContractStmt *CS,
                                                const DeclContext *DC) {
  ContractEvaluationSemantic Sem = CS->ensureRuntimeSemantic(getContext(), DC);
  if (CS->isDynamic()) {
    // The semantic is chosen at run time, here, once for this call; the
    // postcondition's check reuses the choice (DynSelection).
    RawAddress Sel =
        CreateTempAlloca(Int32Ty, CharUnits::fromQuantity(4), "contract.dyn.sel");
    llvm::Value *Eff = EmitDynamicEffectiveSelection(*CS);
    Builder.CreateStore(Eff, Sel);
    ContractData->DynSelection.insert({CS, Sel});
    EmitPostconditionCaptureInit(CS, Sem, Eff);
    return true;
  }
  // 'assume' lowers to 'ignore' (no check emitted, see emitCheckForSemantic):
  // the capture must not be constructed either, since the whole postcondition
  // is gated as a unit.  (D4405: the analysis evaluation never evaluates the
  // predicate, and a runtime evaluation is never prove or diagnose.)
  if (Sem == ContractEvaluationSemantic::Ignore ||
      Sem == ContractEvaluationSemantic::Assume)
    return false;
  EmitPostconditionCaptureInit(CS, Sem);
  return true;
}

llvm::Value *
CodeGenFunction::LoadPostconditionCaptureFailed(const ContractStmt *CS) {
  auto It = ContractData->CaptureInitFailed.find(CS);
  if (It == ContractData->CaptureInitFailed.end())
    return nullptr;
  return Builder.CreateLoad(It->second);
}

// Build the Itanium mangled name for a no-argument C++ selector given a
// (possibly qualified) source name.  The return type is not part of the
// mangling, so we always produce "...Ev".  A bare identifier "foo" mangles to
// "_Z3foov"; a qualified "a::b::foo" mangles to "_ZN1a1b3fooEv".
static std::string MangleDynamicSelectorName(StringRef Name) {
  SmallVector<StringRef, 4> Components;
  Name.split(Components, "::");

  std::string Out = "_Z";
  if (Components.size() == 1) {
    Out += llvm::utostr(Components[0].size());
    Out += Components[0].str();
    Out += 'v';
    return Out;
  }
  Out += 'N';
  for (StringRef C : Components) {
    Out += llvm::utostr(C.size());
    Out += C.str();
  }
  Out += "Ev";
  return Out;
}

// Look up the "C++"-linkage selector NAME and return the declared function
// with the selector's signature that it names, if any.
static const FunctionDecl *lookupDeclaredDynamicSelector(ASTContext &Ctx,
                                                         StringRef Name) {
  const FunctionDecl *Selector = nullptr;
  ContractStmt::lookupDynamicSelector(
      Ctx, Name,
      [&](const NamedDecl *D, ContractStmt::DynamicSelectorMatch M) {
        if (!Selector && M == ContractStmt::DynamicSelectorMatch::Selector)
          Selector = cast<FunctionDecl>(D);
      });
  return Selector;
}

// Get (or create) the dynamic selector function and, when requested, emit a
// weak definition returning ConfiguredSem, the semantic the configuration
// entry specifies (D3595R1).  That is the configured semantic as written, not
// the contract's compile-time semantic: each check adjusts the value its
// selector returns to the allowed set and passes it through the label's
// compute_semantic (getDynTransform), so the weak default reproduces the
// compile-time semantic only if neither has been applied already -- a
// compute_semantic need not be idempotent.  It is also the only value that
// can serve every contract sharing the symbol: the label and the allowed set
// are per contract, the weak definition per symbol, and another translation
// unit, compiled with other flags, may emit the definition the linker keeps.
// Two configuration entries that name one selector with different semantics
// get the first one's here (the first contract to use the selector); give
// them the same semantic.  Deduplicated once per unique symbol per TU.
//
// The selector's ABI matches std::contracts::evaluation_semantic, whose
// underlying type is a 16-bit integer (__UINT16_TYPE__) in both libc++ and
// libstdc++, so it returns i16.  This
// width is fixed at 16 bits deliberately so that a "C"-linkage selector is ABI-
// compatible across toolchains (GCC likewise uses a 16-bit return type here).
// (The return type is not part of the mangled name.)
static llvm::Function *
getOrCreateDynamicSelector(CodeGenModule &CGM, StringRef Name,
                           ContractDynamicLinkage Linkage, bool ProvideWeak,
                           ContractEvaluationSemantic ConfiguredSem) {
  std::string Symbol;
  if (Linkage == ContractDynamicLinkage::C)
    Symbol = Name.str(); // C linkage: verbatim symbol.
  else if (const FunctionDecl *FD =
               lookupDeclaredDynamicSelector(CGM.getContext(), Name))
    Symbol = CGM.getMangledName(GlobalDecl(FD)).str(); // A declared selector.
  else
    Symbol = MangleDynamicSelectorName(Name); // C++ linkage: Itanium mangling.

  llvm::Type *SemTy = llvm::Type::getInt16Ty(CGM.getLLVMContext());
  llvm::FunctionType *FnTy =
      llvm::FunctionType::get(SemTy, /*Params=*/{}, /*isVarArg=*/false);

  llvm::Function *Fn = CGM.getModule().getFunction(Symbol);
  if (!Fn) {
    llvm::FunctionCallee Callee = CGM.CreateRuntimeFunction(FnTy, Symbol);
    Fn = cast<llvm::Function>(Callee.getCallee());
  }

  // Emit the weak fallback once, if requested and not already defined.
  if (ProvideWeak && Fn->isDeclaration()) {
    Fn->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
    llvm::BasicBlock *Entry =
        llvm::BasicBlock::Create(CGM.getLLVMContext(), "entry", Fn);
    llvm::IRBuilder<> B(Entry);
    B.CreateRet(
        llvm::ConstantInt::get(SemTy, static_cast<int>(ConfiguredSem)));
  }

  return Fn;
}

void CodeGenFunction::EmitContractStmtAsFullStmt(const ContractStmt &S) {
  noteContractEvaluation(S);

  const DeclContext *FnCtx =
      CurFuncDecl ? CurFuncDecl->getDeclContext() : nullptr;
  ContractEvaluationSemantic Semantic = S.ensureRuntimeSemantic(getContext(),
                                                                FnCtx);

  // D4405: the analysis evaluation comes first, on its own, ahead of the
  // runtime evaluation and whatever its check lets a proof assume, so a proof
  // never relies on the assertion's own check.  A precondition's is at each
  // call instead (DiagnoseCallerSideProvingContracts), but in C.  It has no
  // run-time effect: the evaluation is diagnosed, or deferred to a proof
  // shadow, whose real-code assumption (-fcontracts-prove-assume) then lets
  // the optimizers drop the check that follows.
  bool Analyzed = false;
  // D4405: a virtual function's postcondition is analyzed first for each
  // override pair, ahead of its own analysis evaluation.
  emitContractOverrideOwnPost(S);
  if (isContractAnalyzedHere(S)) {
    ContractEvaluationSemantic Analysis =
        S.ensureAnalysisSemantic(getContext(), FnCtx);
    if (isProvingContractSemantic(Analysis)) {
      CGM.diagnoseUnprovenContract(S, Analysis, /*CallSite=*/nullptr,
                                   /*Callee=*/nullptr, this);
      Analyzed = true;
    }
  }

  // P3595 dynamic selection: when the contract's runtime semantic is chosen by
  // a selector function at link/run time, emit the selector call plus a switch
  // that dispatches, per raw return value, to the precomputed effective
  // semantic's check body.  Sema already folded the label transform into the
  // per-value table (getDynTransform); codegen just consumes it.
  if (S.isDynamic()) {
    // A postcondition with captures chose its semantic where the captures
    // were initialized (EmitPostconditionCaptures): dispatch on that choice,
    // already mapped to the effective semantic, rather than call the selector
    // again.
    Address StoredSel = Address::invalid();
    if (auto It = ContractData->DynSelection.find(&S);
        It != ContractData->DynSelection.end())
      StoredSel = It->second;

    llvm::Value *RawExt;
    EnsureInsertPoint();
    if (StoredSel.isValid()) {
      RawExt = Builder.CreateLoad(StoredSel, "contract.dyn.eff");
    } else {
      // Selector call.  The weak fallback returns the configured semantic,
      // which the switch below adjusts like any value the selector returns.
      // An ordinary call: an exception from the selector unwinds through
      // this function's cleanups and handlers like any other.
      llvm::Function *Selector = getOrCreateDynamicSelector(
          CGM, S.getDynName(), S.getDynLinkage(), S.getDynProvideWeak(),
          S.getDynConfiguredSemantic());
      llvm::Value *Raw = EmitCallOrInvoke(Selector, {}, "contract.dyn.raw");
      // Widen to the switch condition type (i32) for a uniform comparison.
      RawExt = Builder.CreateZExt(Raw, IntTy, "contract.dyn.sem");
    }

    // Continuation block that all non-terminating arms rejoin.
    llvm::BasicBlock *Continue = createBasicBlock("contract.dyn.continue");

    // The enforced-violation block: an unconditional enforce handler call with
    // this contract's violation info.  Reached by the sentinel (disallowed
    // transform) arms and by 'default' (unknown raw value).  The shared enforce
    // block already emits the enforce handler + unreachable; we just add this
    // contract's data pointer to its PHI and branch to it.
    llvm::Constant *EnforceInfo = BuildContractViolationInfo(*this, S);
    llvm::BasicBlock *EnforceBB = createBasicBlock("contract.dyn.enforced");

    llvm::SwitchInst *Switch =
        Builder.CreateSwitch(RawExt, EnforceBB,
                             /*NumCases=*/ContractStmt::NumDynRawSemantics);

    // Build one arm per raw value.  Deduplicate arm blocks by effective
    // semantic so each distinct check body is emitted at most once.
    llvm::DenseMap<int, llvm::BasicBlock *> SemToBlock;
    for (unsigned R = 1; R <= ContractStmt::NumDynRawSemantics; ++R) {
      // A stored selection is already the effective semantic, which is never
      // prove or diagnose (getDynTransform records them as ignore).
      ContractEvaluationSemantic Eff =
          StoredSel.isValid() ? static_cast<ContractEvaluationSemantic>(R)
                              : S.getDynTransform(R);
      if (isProvingContractSemantic(Eff))
        Eff = ContractEvaluationSemantic::Ignore;
      if (static_cast<int>(Eff) == 0) {
        // Sentinel: disallowed transform -> enforced violation.
        Switch->addCase(llvm::ConstantInt::get(IntTy, R), EnforceBB);
        continue;
      }
      auto It = SemToBlock.find(static_cast<int>(Eff));
      llvm::BasicBlock *ArmBB;
      if (It != SemToBlock.end()) {
        ArmBB = It->second;
      } else {
        ArmBB = createBasicBlock("contract.dyn.arm");
        SemToBlock[static_cast<int>(Eff)] = ArmBB;
        EmitBlock(ArmBB);
        emitCheckForSemantic(S, Eff, Continue);
      }
      Switch->addCase(llvm::ConstantInt::get(IntTy, R), ArmBB);
    }

    // Emit the enforced-violation block.  With exceptions on, the entry-point
    // call is an invoke into the EH scope it is emitted in, so it is emitted
    // here, in this contract's scope: the shared block is built in the scope
    // of whichever contract asks for it first, and a handler's exception from
    // a later contract would unwind to that first contract's landing pad.
    // Without exceptions, hand off to the shared enforce block for this
    // contract's kind (so the violation is reported as pre/post/assert rather
    // than a single hardcoded kind).  When the allowed set holds no throwing
    // semantic (contractSemanticsNothrow, DECISIONS.md E137 and E138), no
    // exception may leave the check: with -fcontracts-p4298 the violation is
    // a noexcept_enforce one, emitted here too, so a handler's exception
    // terminates; without it, where noexcept_enforce is not available, it is
    // a quick_enforce trap calling no handler, whether or not the label
    // allows quick_enforce.
    EmitBlock(EnforceBB);
    ContractKind Kind = S.getContractKind();
    const LangOptions &LO = getLangOpts();
    bool Nothrow = contractSemanticsNothrow(
        S.getAllowedMask() &
        gatedContractSemanticsMask(LO.ContractOpts.AllowAssume,
                                   LO.ContractsP4298, LO.ContractsP4405));
    if (Nothrow && !LO.ContractsP4298) {
      Builder.CreateBr(GetSharedContractViolationTrapBlock());
    } else if (Nothrow) {
      llvm::Value *DataPtr = emitContractDataBlock(*this, S, EnforceInfo);
      EmitCxaContractViolationCall(Kind, NoexceptEnforce, PredicateFailed,
                                   DataPtr, /*IsNoExcept=*/true,
                                   /*IsPostCapture=*/false);
      EnsureInsertPoint();
    } else if (getLangOpts().Exceptions) {
      llvm::Value *DataPtr = emitContractDataBlock(*this, S, EnforceInfo);
      EmitCxaContractViolationCall(Kind, ContractEvaluationSemantic::Enforce,
                                   PredicateFailed, DataPtr,
                                   /*IsNoExcept=*/false,
                                   /*IsPostCapture=*/false);
      EnsureInsertPoint();
    } else {
      ContractData->GetSharedEnforceBlock(*this, Kind)
          .IncomingPHI->addIncoming(EnforceInfo, EnforceBB);
      Builder.CreateBr(GetSharedContractViolationEnforceBlock(Kind));
    }

    EmitBlock(Continue);
    return;
  }

  // An ignore runtime evaluation after an analysis one places no assumption
  // in a proof shadow: it would only make the shadow conditional, and the
  // analysis evaluation's obligation gives the predicate already.
  if (Analyzed && Semantic == ContractEvaluationSemantic::Ignore)
    return;
  emitCheckForSemantic(S, Semantic, /*ContinueBlock=*/nullptr);
}

/// Build the per-assertion data block the runtime walks, as a stack copy of
/// the global block with the label's facet pointers appended.  Returns the
/// pointer to hand the entry point: the extended block when the label carries
/// a local handler or a query, and the plain global block otherwise.
///
/// Both detection paths need this -- the predicate-false path and the
/// exception path -- or a throwing predicate is reported through a block
/// carrying neither facet: the local violation handler is not dispatched to,
/// and query_control_object answers null.  (GCC passes one shared block to
/// both entry points.)
///
/// Built at the use site rather than hoisted above the try: both callers are
/// cold (a violation has already been detected), whereas hoisting would put
/// the label expression's evaluation on the hot path of every check.
static llvm::Value *emitContractDataBlock(CodeGenFunction &CGF,
                                          const ContractStmt &S,
                                          llvm::Value *BasicBlock) {
  bool HasLocalHandler = S.hasLocalHandler() && S.getLabelExpr();
  bool HasQuery = S.hasQuery() && S.getLabelExpr();
  if (!HasLocalHandler && !HasQuery)
    return BasicBlock;

  CodeGenModule &CGM = CGF.CGM;
  llvm::Function *HandlerTrampoline =
      HasLocalHandler ? getFacetTrampoline(CGM, S.getLocalHandlerFn())
                      : nullptr;
  llvm::Function *QueryTrampoline =
      HasQuery ? getFacetTrampoline(CGM, S.getQueryFn()) : nullptr;

  bool EmitHandler = HandlerTrampoline != nullptr;
  bool EmitQuery = QueryTrampoline != nullptr;
  if (!EmitHandler && !EmitQuery)
    return BasicBlock;

  auto &Builder = CGF.Builder;
  llvm::Value *LabelAddr = CGF.EmitLValue(S.getLabelExpr()).getPointer(CGF);

  // Extended struct: 8 basic fields + handler? + query? + label_ptr.
  llvm::Type *PtrTy = CGM.VoidPtrTy;
  llvm::Type *I32Ty = llvm::Type::getInt32Ty(CGF.getLLVMContext());
  SmallVector<llvm::Type *, 11> FieldTypes = {PtrTy, PtrTy, PtrTy, PtrTy,
                                              I32Ty, I32Ty, PtrTy, PtrTy};
  if (EmitHandler)
    FieldTypes.push_back(PtrTy);
  if (EmitQuery)
    FieldTypes.push_back(PtrTy);
  FieldTypes.push_back(PtrTy); // label_ptr

  llvm::StructType *ExtBlockTy =
      llvm::StructType::get(CGF.getLLVMContext(), FieldTypes,
                            /*isPacked=*/false);

  Address ExtBlock = CGF.CreateTempAlloca(
      ExtBlockTy, CharUnits::fromQuantity(8), "contract.ext.data");

  llvm::Constant *ExtDescTable =
      getOrCreateDescriptorTable(CGM, EmitHandler, EmitQuery);
  llvm::Value *ExtBlockRaw = ExtBlock.emitRawPointer(CGF);
  const llvm::DataLayout &DL = CGM.getModule().getDataLayout();

  for (unsigned i = 0; i < 8; ++i) {
    llvm::Value *DstFieldPtr =
        Builder.CreateStructGEP(ExtBlockTy, ExtBlockRaw, i);
    if (i == 0) {
      Builder.CreateDefaultAlignedStore(ExtDescTable, DstFieldPtr);
    } else {
      llvm::Value *SrcFieldPtr =
          Builder.CreateStructGEP(ExtBlockTy, BasicBlock, i);
      llvm::Value *Val = Builder.CreateAlignedLoad(
          ExtBlockTy->getStructElementType(i), SrcFieldPtr,
          CharUnits::fromQuantity(
              DL.getABITypeAlign(ExtBlockTy->getStructElementType(i)).value()));
      Builder.CreateDefaultAlignedStore(Val, DstFieldPtr);
    }
  }

  unsigned ExtIdx = 8;
  if (EmitHandler) {
    llvm::Value *Field =
        Builder.CreateStructGEP(ExtBlockTy, ExtBlockRaw, ExtIdx++);
    Builder.CreateDefaultAlignedStore(HandlerTrampoline, Field);
  }
  if (EmitQuery) {
    llvm::Value *Field =
        Builder.CreateStructGEP(ExtBlockTy, ExtBlockRaw, ExtIdx++);
    Builder.CreateDefaultAlignedStore(QueryTrampoline, Field);
  }
  // label_ptr is always last.
  llvm::Value *LabelField =
      Builder.CreateStructGEP(ExtBlockTy, ExtBlockRaw, ExtIdx);
  Builder.CreateDefaultAlignedStore(LabelAddr, LabelField);

  return ExtBlockRaw;
}

// -------------------------------------------------------------------
// Rethrow shortcut analysis (quality of implementation).
//
// A P3400 local violation handler that responds to an evaluation_exception
// detection by rethrowing the in-flight exception makes the EH region we wrap
// around a possibly-throwing predicate pure overhead: the exception is caught
// only to be handed to a handler that throws it straight back out.  This
// analysis recognizes that shape so the caller can skip the region and let the
// predicate's exception propagate on its own.
//
// Equivalence rests on the local handler running before the global one and
// short-circuiting it (libcontracts/dispatch.c): if the local handler
// rethrows, nothing else observable happens between the catch and the rethrow,
// and no violation is ever reported.  It holds only for the enforce and
// observe semantics -- quick_enforce calls no handler, and the P4298 noexcept_
// semantics exist precisely to guarantee nothing propagates.
//
// The walk follows calls, which is what lets it see through delegation: a
// handler that calls a helper whose body is just `throw;', and the
// __combined_label handler that forwards to the component labels, both come
// out of the same mechanism rather than being special-cased.
//
// Everything here is conservative: any construct it does not model makes it
// answer "no", leaving the EH region in place.
//
// Mirror of gnu_gcc's contract_local_handler_always_rethrows_p.  One
// deliberate divergence: GCC has to force template instantiation to get the
// combined handler's body, because it runs during genericization while the
// definition is still only queued.  Clang performs pending instantiations
// before CodeGen, so the body is simply there.
// -------------------------------------------------------------------

namespace {

/// An abstract value tracked while walking a handler body.
enum AValKind {
  AV_Unknown,          ///< Nothing known.
  AV_Const,            ///< A known integer, enumeration or boolean value.
  AV_CurrentException, ///< The result of std::current_exception().
};

struct AVal {
  AValKind Kind = AV_Unknown;
  int64_t Val = 0;
};

static AVal avUnknown() { return AVal{AV_Unknown, 0}; }
static AVal avConst(int64_t V) { return AVal{AV_Const, V}; }
static AVal avCurrentException() { return AVal{AV_CurrentException, 0}; }

/// How control left a statement, under the analysis assumption.
///
/// RO_Returned is distinct from RO_Fail because the two mean different things
/// depending on whose frame the walk is in.  For the handler itself, returning
/// is a failure: it did not rethrow.  For a function the handler called,
/// returning is a success of sorts -- the call completed without doing
/// anything observable, so the walk of the caller carries on past it.
enum RethrowOutcome {
  RO_Fallthrough, ///< Control continues with the next statement.
  RO_Rethrown,    ///< Control left by rethrowing the in-flight exception.
  RO_Returned,    ///< Control returned normally, having done nothing
                  ///< observable.
  RO_Break,       ///< Control left the innermost enclosing switch by a break.
  RO_Fail,        ///< Could not be analysed, or something observable happened.
};

/// How deep the walk will follow calls before giving up.  A handler that
/// delegates more than this far is not a shape worth proving, and the limit
/// doubles as the termination guard for mutual recursion.
static const int RethrowMaxDepth = 8;

/// One analysis of one handler body, under one (semantic, kind) pair.  The
/// walk assumes the violation was detected as ExceptionRaised.
class RethrowAnalysis {
public:
  RethrowAnalysis(const ParmVarDecl *ViolationParm,
                  ContractEvaluationSemantic Semantic, int64_t KindValue,
                  int Depth = 0)
      : ViolationParm(ViolationParm), Semantic(Semantic), KindValue(KindValue),
        Depth(Depth) {}

  RethrowOutcome walkStmt(const Stmt *S);

  /// Valid after walkStmt returned RO_Returned: what the function returned, as
  /// far as the abstract domain could tell.
  AVal returnedValue() const { return Returned; }

private:
  RethrowOutcome walkSwitch(const SwitchStmt *Sw);
  AVal eval(const Expr *E);
  bool accessorValue(const CXXMemberCallExpr *Call, AVal &Out);
  RethrowOutcome callOutcome(const CallExpr *Call, AVal &ValueOut);

  /// Evaluate E for its value, insisting that getting there costs nothing
  /// observable.  False means the expression is not something the domain can
  /// account for, and the statement containing it must not be walked past.
  bool evalPure(const Expr *E, AVal &Out) {
    Impure = false;
    Rethrew = false;
    Out = eval(E);
    return !Impure;
  }

  /// Evaluate E where a value is wanted and control is expected to carry on.
  /// RO_Fallthrough means Out holds it; RO_Rethrown means evaluating E never
  /// produced a value at all, because something it called rethrew.
  RethrowOutcome evalValue(const Expr *E, AVal &Out) {
    if (evalPure(E, Out))
      return RO_Fallthrough;
    return Rethrew ? RO_Rethrown : RO_Fail;
  }

  const ParmVarDecl *ViolationParm;
  ContractEvaluationSemantic Semantic;
  int64_t KindValue;
  int Depth;
  AVal Returned;

  /// Set by eval when it meets something it cannot account for.  AV_Unknown
  /// alone does not mean "unmodelled" -- reading an untracked local yields an
  /// unknown value from a perfectly pure expression -- so a caller willing to
  /// carry on with an unknown value still has to know whether getting there
  /// cost anything observable.
  bool Impure = false;

  /// Set alongside Impure when the reason no value came back is that a call
  /// inside the expression always rethrows.
  bool Rethrew = false;

  /// Local scalar VarDecl -> abstract value.
  llvm::DenseMap<const VarDecl *, AVal> Env;

  /// How many switches the walk is inside: a break leaves the innermost one,
  /// and anywhere else it is a jump this walk does not model.
  unsigned SwitchDepth = 0;
};

/// If CALL invokes one of the contract_violation accessors whose result is
/// known at the point the check is emitted, store that value in OUT and return
/// true.  The call must be on the handler's own violation parameter: a
/// different contract_violation object tells us nothing.
bool RethrowAnalysis::accessorValue(const CXXMemberCallExpr *Call, AVal &Out) {
  const CXXMethodDecl *MD = Call->getMethodDecl();
  if (!MD || !MD->getDeclName().isIdentifier() || !ViolationParm)
    return false;

  const CXXRecordDecl *RD = MD->getParent();
  if (!RD || RD->getName() != "contract_violation")
    return false;

  const auto *Obj = dyn_cast_or_null<DeclRefExpr>(
      Call->getImplicitObjectArgument()->IgnoreParenImpCasts());
  if (!Obj || Obj->getDecl() != ViolationParm)
    return false;

  StringRef Name = MD->getName();
  if (Name == "detection_mode")
    Out = avConst(static_cast<int64_t>(ContractDetectionMode::ExceptionRaised));
  else if (Name == "semantic")
    Out = avConst(static_cast<int64_t>(Semantic));
  else if (Name == "kind")
    Out = avConst(KindValue);
  else if (Name == "is_terminating")
    // Of the two semantics this analysis runs for, only enforce terminates.
    Out = avConst(Semantic == Enforce);
  else
    return false;

  return true;
}

/// Walk into CALL's callee and report how control leaves the call.
///
/// RO_Rethrown means the callee always rethrows the in-flight exception, so
/// the call is as good as a `throw;' written here.  RO_Returned means it always
/// returns having done nothing observable, so the caller's walk carries on;
/// ValueOut then holds the returned value where that could be folded.
///
/// The recursion is the same predicate applied one frame down, so "did nothing
/// else observable first" is enforced at every level for free.
RethrowOutcome RethrowAnalysis::callOutcome(const CallExpr *Call,
                                            AVal &ValueOut) {
  ValueOut = avUnknown();

  if (Depth >= RethrowMaxDepth)
    return RO_Fail;

  const FunctionDecl *FD = Call->getDirectCallee();
  if (!FD)
    return RO_Fail;

  // Calling a function with contracts evaluates them too; a weak definition
  // may be replaced at link time; and a virtual call runs the dynamic
  // overrider, not the method named, unless the call is qualified.
  if (FD->hasContracts() || FD->isWeak())
    return RO_Fail;
  if (const auto *MC = dyn_cast<CXXMemberCallExpr>(Call))
    if (const CXXMethodDecl *MD = MC->getMethodDecl(); MD && MD->isVirtual()) {
      const auto *ME = dyn_cast<MemberExpr>(MC->getCallee()->IgnoreParens());
      if (!ME || !ME->hasQualifier())
        return RO_Fail;
    }

  const Stmt *Body = FD->getBody();
  if (!Body)
    return RO_Fail;

  // Find the callee parameter, if any, that received our violation object, so
  // the accessors keep folding across the delegation.  When none does -- the
  // `void helper() { throw; }' shape -- the nested walk simply runs without a
  // violation parameter, which is all such a helper needs.
  const ParmVarDecl *NestedParm = nullptr;
  if (ViolationParm) {
    unsigned N = std::min<unsigned>(Call->getNumArgs(), FD->getNumParams());
    for (unsigned I = 0; I != N; ++I) {
      const auto *Arg =
          dyn_cast<DeclRefExpr>(Call->getArg(I)->IgnoreParenImpCasts());
      if (Arg && Arg->getDecl() == ViolationParm) {
        NestedParm = FD->getParamDecl(I);
        break;
      }
    }
  }

  RethrowAnalysis Nested(NestedParm, Semantic, KindValue, Depth + 1);
  switch (Nested.walkStmt(Body)) {
  case RO_Rethrown:
    // A rethrow out of a noexcept callee terminates rather than propagating,
    // which is not what skipping the EH region would do.
    if (FD->getType()->castAs<FunctionProtoType>()->isNothrow())
      return RO_Fail;
    return RO_Rethrown;
  case RO_Returned:
    ValueOut = Nested.returnedValue();
    return RO_Returned;
  case RO_Fallthrough:
    // Ran off the end of a void body: it returned, with no value.
    return RO_Returned;
  default:
    return RO_Fail;
  }
}

/// Evaluate E as far as the abstract domain allows.
AVal RethrowAnalysis::eval(const Expr *E) {
  if (!E)
    return avUnknown();

  E = E->IgnoreParens();

  // A constexpr-if condition arrives already folded, as does anything else
  // Clang could evaluate; take the value rather than re-deriving it.
  if (const auto *CE = dyn_cast<ConstantExpr>(E))
    if (CE->hasAPValueResult() && CE->getAPValueResult().isInt())
      return avConst(CE->getAPValueResult().getInt().getSExtValue());

  if (const auto *IL = dyn_cast<IntegerLiteral>(E))
    return avConst(IL->getValue().getSExtValue());
  if (const auto *BL = dyn_cast<CXXBoolLiteralExpr>(E))
    return avConst(BL->getValue());

  if (const auto *DRE = dyn_cast<DeclRefExpr>(E)) {
    if (const auto *ECD = dyn_cast<EnumConstantDecl>(DRE->getDecl()))
      return avConst(ECD->getInitVal().getSExtValue());
    // A volatile access is observable behaviour ([intro.abstract]).
    if (DRE->getType().isVolatileQualified()) {
      Impure = true;
      return avUnknown();
    }
    if (const auto *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      auto It = Env.find(VD);
      if (It != Env.end())
        return It->second;
    }
    return avUnknown();
  }

  if (const auto *ICE = dyn_cast<ImplicitCastExpr>(E))
    return eval(ICE->getSubExpr());
  if (const auto *ECE = dyn_cast<ExplicitCastExpr>(E))
    return eval(ECE->getSubExpr());
  if (const auto *EWC = dyn_cast<ExprWithCleanups>(E))
    return eval(EWC->getSubExpr());
  if (const auto *BTE = dyn_cast<CXXBindTemporaryExpr>(E))
    return eval(BTE->getSubExpr());
  if (const auto *MTE = dyn_cast<MaterializeTemporaryExpr>(E))
    return eval(MTE->getSubExpr());

  // Reading an object is not observable, unless it is volatile
  // ([intro.abstract]); __combined_label reaches its components through
  // this->First.
  if (isa<CXXThisExpr>(E))
    return avUnknown();
  if (const auto *ME = dyn_cast<MemberExpr>(E)) {
    if (ME->getType().isVolatileQualified()) {
      Impure = true;
      return avUnknown();
    }
    eval(ME->getBase());
    return avUnknown();
  }

  if (const auto *UO = dyn_cast<UnaryOperator>(E)) {
    if (UO->getOpcode() == UO_Deref && !UO->getType().isVolatileQualified()) {
      eval(UO->getSubExpr());
      return avUnknown();
    }
    if (UO->getOpcode() == UO_LNot) {
      AVal A = eval(UO->getSubExpr());
      return A.Kind == AV_Const ? avConst(!A.Val) : avUnknown();
    }
    Impure = true;
    return avUnknown();
  }

  if (const auto *BO = dyn_cast<BinaryOperator>(E)) {
    switch (BO->getOpcode()) {
    case BO_LAnd: {
      AVal L = eval(BO->getLHS());
      if (L.Kind == AV_Const && !L.Val)
        return avConst(0);
      AVal R = eval(BO->getRHS());
      if (L.Kind != AV_Const || R.Kind != AV_Const)
        return avUnknown();
      return avConst(L.Val && R.Val);
    }
    case BO_LOr: {
      AVal L = eval(BO->getLHS());
      if (L.Kind == AV_Const && L.Val)
        return avConst(1);
      AVal R = eval(BO->getRHS());
      if (L.Kind != AV_Const || R.Kind != AV_Const)
        return avUnknown();
      return avConst(L.Val || R.Val);
    }
    case BO_EQ:
    case BO_NE:
    case BO_LT:
    case BO_LE:
    case BO_GT:
    case BO_GE: {
      AVal L = eval(BO->getLHS());
      AVal R = eval(BO->getRHS());
      if (L.Kind != AV_Const || R.Kind != AV_Const)
        return avUnknown();
      switch (BO->getOpcode()) {
      case BO_EQ:
        return avConst(L.Val == R.Val);
      case BO_NE:
        return avConst(L.Val != R.Val);
      case BO_LT:
        return avConst(L.Val < R.Val);
      case BO_LE:
        return avConst(L.Val <= R.Val);
      case BO_GT:
        return avConst(L.Val > R.Val);
      default:
        return avConst(L.Val >= R.Val);
      }
    }
    default:
      Impure = true;
      return avUnknown();
    }
  }

  if (const auto *Call = dyn_cast<CallExpr>(E)) {
    AVal A;
    if (const auto *MC = dyn_cast<CXXMemberCallExpr>(Call))
      if (accessorValue(MC, A))
        return A;

    if (const FunctionDecl *FD = Call->getDirectCallee())
      if (FD->getDeclName().isIdentifier() &&
          FD->getName() == "current_exception" && FD->isInStdNamespace())
        return avCurrentException();

    // The arguments are evaluated first, the object of a member call among
    // them; a side effect there is observable whatever the callee does.
    if (const auto *MC = dyn_cast<CXXMemberCallExpr>(Call))
      eval(MC->getImplicitObjectArgument());
    for (const Expr *Arg : Call->arguments())
      eval(Arg);
    if (Impure)
      return avUnknown();

    // Otherwise follow the callee.  It may return something knowable having
    // done nothing observable, in which case the value stands in for the call;
    // or it may always rethrow, in which case the expression yields no value
    // and the statement containing it has to be told.
    AVal V;
    switch (callOutcome(Call, V)) {
    case RO_Returned:
      return V;
    case RO_Rethrown:
      Rethrew = true;
      Impure = true;
      return avUnknown();
    default:
      Impure = true;
      return avUnknown();
    }
  }

  Impure = true;
  return avUnknown();
}

/// Walk statement S under the assumption that the violation was detected as
/// ExceptionRaised, reporting how control leaves it.
RethrowOutcome RethrowAnalysis::walkStmt(const Stmt *S) {
  if (!S)
    return RO_Fallthrough;

  if (isa<NullStmt>(S))
    return RO_Fallthrough;

  if (const auto *CS = dyn_cast<CompoundStmt>(S)) {
    for (const Stmt *Sub : CS->body()) {
      RethrowOutcome O = walkStmt(Sub);
      if (O != RO_Fallthrough)
        return O;
    }
    return RO_Fallthrough;
  }

  if (const auto *AS = dyn_cast<AttributedStmt>(S)) {
    // Look through only the attributes known to have no run-time effect: an
    // [[assume]] is a checked assertion under -fcontracts-p3100.
    for (const Attr *A : AS->getAttrs())
      if (!isa<LikelyAttr, UnlikelyAttr, FallThroughAttr>(A))
        return RO_Fail;
    return walkStmt(AS->getSubStmt());
  }

  if (const auto *DS = dyn_cast<DeclStmt>(S)) {
    for (const Decl *D : DS->decls()) {
      // Type aliases carry no code -- __combined_label's `using _Ret = ...'
      // arrives here.
      if (isa<TypeDecl>(D) || isa<UsingDecl>(D) || isa<StaticAssertDecl>(D))
        continue;
      const auto *VD = dyn_cast<VarDecl>(D);
      if (!VD || VD->hasGlobalStorage())
        return RO_Fail;
      // Only scalars: a class-typed local brings a destructor, and with it
      // cleanup control flow this walk does not model.
      if (!VD->getType()->isScalarType())
        return RO_Fail;
      AVal Init = avUnknown();
      if (const Expr *E = VD->getInit()) {
        RethrowOutcome O = evalValue(E, Init);
        if (O != RO_Fallthrough)
          return O;
      }
      Env[VD] = Init;
    }
    return RO_Fallthrough;
  }

  if (const auto *If = dyn_cast<IfStmt>(S)) {
    // An `if constexpr' in an instantiated template arrives with its condition
    // already folded and the discarded branch left empty, so the same
    // fold-and-take-one-branch logic covers both forms.
    if (If->getInit() || If->getConditionVariable())
      return RO_Fail;
    AVal C = avUnknown();
    RethrowOutcome O = evalValue(If->getCond(), C);
    if (O != RO_Fallthrough)
      return O;
    if (C.Kind != AV_Const)
      return RO_Fail;
    return walkStmt(C.Val ? If->getThen() : If->getElse());
  }

  if (const auto *Sw = dyn_cast<SwitchStmt>(S))
    return walkSwitch(Sw);

  if (isa<BreakStmt>(S))
    return SwitchDepth ? RO_Break : RO_Fail;

  if (const auto *Ret = dyn_cast<ReturnStmt>(S)) {
    // Record what was returned, for a caller that is following this call.
    AVal V = avUnknown();
    if (const Expr *RV = Ret->getRetValue()) {
      RethrowOutcome O = evalValue(RV, V);
      if (O != RO_Fallthrough)
        return O;
    }
    Returned = V;
    return RO_Returned;
  }

  // An expression statement: a rethrow, a modelled call, or an assignment to a
  // local we are tracking.
  if (const auto *E = dyn_cast<Expr>(S)) {
    const Expr *Inner = E->IgnoreParens();
    if (const auto *EWC = dyn_cast<ExprWithCleanups>(Inner))
      Inner = EWC->getSubExpr()->IgnoreParens();

    // `throw;' is a CXXThrowExpr with no operand.  `throw X;' raises a
    // different exception, which is not what skipping the region would do.
    if (const auto *Throw = dyn_cast<CXXThrowExpr>(Inner))
      return Throw->getSubExpr() ? RO_Fail : RO_Rethrown;

    // std::rethrow_exception(std::current_exception()) rethrows the exception
    // that is in flight, so it reaches the same place.
    if (const auto *Call = dyn_cast<CallExpr>(Inner))
      if (const FunctionDecl *FD = Call->getDirectCallee())
        if (FD->getDeclName().isIdentifier() &&
            FD->getName() == "rethrow_exception" && FD->isInStdNamespace() &&
            Call->getNumArgs() == 1 &&
            eval(Call->getArg(0)).Kind == AV_CurrentException)
          return RO_Rethrown;

    if (const auto *BO = dyn_cast<BinaryOperator>(Inner)) {
      if (BO->getOpcode() != BO_Assign)
        return RO_Fail;
      // Only assignments to locals we are already tracking; a store anywhere
      // else is an observable effect.
      const auto *LHS =
          dyn_cast<DeclRefExpr>(BO->getLHS()->IgnoreParenImpCasts());
      const auto *VD = LHS ? dyn_cast<VarDecl>(LHS->getDecl()) : nullptr;
      if (!VD || !Env.count(VD))
        return RO_Fail;
      AVal RHS = avUnknown();
      RethrowOutcome O = evalValue(BO->getRHS(), RHS);
      if (O != RO_Fallthrough)
        return O;
      Env[VD] = RHS;
      return RO_Fallthrough;
    }

    // Any other discarded-value expression: let eval decide, which follows
    // calls and reports a rethrow.
    AVal Discarded = avUnknown();
    return evalValue(Inner, Discarded);
  }

  return RO_Fail;
}

/// Append S to Out as the flat sequence a switch's case labels sit in: each
/// label (CaseStmt or DefaultStmt) itself, followed by the flattening of the
/// statement it labels.  Statements are not descended into otherwise.
static void flattenSwitchBody(const Stmt *S,
                              SmallVectorImpl<const Stmt *> &Out) {
  if (!S)
    return;
  Out.push_back(S);
  if (const auto *SC = dyn_cast<SwitchCase>(S))
    flattenSwitchBody(SC->getSubStmt(), Out);
}

/// Whether S contains a case or default label of the switch being walked,
/// that is, one nested inside a statement rather than at the switch body's
/// top level.  An inner switch's labels are its own.
static bool containsNestedSwitchLabel(const Stmt *S) {
  if (!S || isa<SwitchStmt>(S))
    return false;
  if (isa<SwitchCase>(S))
    return true;
  for (const Stmt *Child : S->children())
    if (containsNestedSwitchLabel(Child))
      return true;
  return false;
}

/// Walk Sw: fold its condition, find the case label it selects (or the
/// default), and walk on from there through the labels that follow, which
/// control falls through, until a break leaves the switch.  No matching label
/// and no default skips the body.  Anything irregular -- a condition the
/// domain cannot fold, a case label nested inside a statement, a jump other
/// than a break -- fails.
RethrowOutcome RethrowAnalysis::walkSwitch(const SwitchStmt *Sw) {
  if (Sw->getInit() || Sw->getConditionVariable())
    return RO_Fail;
  AVal C = avUnknown();
  RethrowOutcome O = evalValue(Sw->getCond(), C);
  if (O != RO_Fallthrough)
    return O;
  if (C.Kind != AV_Const)
    return RO_Fail;

  SmallVector<const Stmt *, 16> Body;
  if (const auto *CS = dyn_cast_or_null<CompoundStmt>(Sw->getBody())) {
    for (const Stmt *Sub : CS->body())
      flattenSwitchBody(Sub, Body);
  } else {
    flattenSwitchBody(Sw->getBody(), Body);
  }

  // The case labels must all be at the top level.
  std::optional<unsigned> Start, Default;
  for (unsigned I = 0, N = Body.size(); I != N; ++I) {
    const Stmt *St = Body[I];
    if (const auto *Case = dyn_cast<CaseStmt>(St)) {
      if (Start)
        continue;
      AVal Lo = eval(Case->getLHS());
      AVal Hi = Case->getRHS() ? eval(Case->getRHS()) : Lo;
      if (Lo.Kind != AV_Const || Hi.Kind != AV_Const)
        return RO_Fail;
      if (Lo.Val <= C.Val && C.Val <= Hi.Val)
        Start = I;
    } else if (isa<DefaultStmt>(St)) {
      Default = I;
    } else if (containsNestedSwitchLabel(St)) {
      return RO_Fail;
    }
  }
  if (!Start)
    Start = Default;
  if (!Start)
    return RO_Fallthrough;

  ++SwitchDepth;
  O = RO_Fallthrough;
  for (unsigned I = *Start, N = Body.size(); I != N && O == RO_Fallthrough; ++I)
    if (!isa<SwitchCase>(Body[I]))
      O = walkStmt(Body[I]);
  --SwitchDepth;
  return O == RO_Break ? RO_Fallthrough : O;
}

} // anonymous namespace

/// True if S's label carries a local violation handler that, for a violation
/// detected as ExceptionRaised under SEMANTIC, always exits by rethrowing the
/// in-flight exception without first doing anything else observable.  When it
/// does, the caller may skip wrapping the predicate in an EH region: the
/// exception reaches the same place either way.
///
/// Conservative -- false whenever this cannot be proven.
static bool contractLocalHandlerAlwaysRethrows(CodeGenFunction &CGF,
                                               const ContractStmt &S,
                                               ContractEvaluationSemantic Sem,
                                               bool IsPostCapture) {
  if (!CGF.getLangOpts().ContractBypassRethrowingLocalHandler)
    return false;

  // Only the two semantics whose handler may legitimately let an exception
  // escape.
  if (Sem != Enforce && Sem != Observe)
    return false;

  if (!S.hasLocalHandler() || !S.getLabelExpr())
    return false;

  const CXXMethodDecl *HCV = findLocalViolationHandler(S);
  if (!HCV || HCV->isVirtual())
    return false;

  // A noexcept handler cannot rethrow -- it would terminate.
  if (HCV->getType()->castAs<FunctionProtoType>()->isNothrow())
    return false;

  // Calling a handler with contracts evaluates them before its body.
  if (HCV->hasContracts())
    return false;

  const Stmt *Body = HCV->getBody();
  if (!Body || HCV->getNumParams() != 1)
    return false;

  RethrowAnalysis Analysis(
      HCV->getParamDecl(0), Sem,
      static_cast<int64_t>(
          getContractAssertionKind(S.getContractKind(), IsPostCapture)));
  return Analysis.walkStmt(Body) == RO_Rethrown;
}

void CodeGenFunction::emitCheckForSemantic(const ContractStmt &S,
                                           ContractEvaluationSemantic Semantic,
                                           llvm::BasicBlock *ContinueBlock) {
  // P4405 proving hook: in a proof shadow, an assumption.
  // Elsewhere, a terminating check is noted as a premise of this function.
  if (emitContractShadowAssumption(S, Semantic, ContinueBlock != nullptr))
    return;

  // P3100 "assume" deliberately emits no check today, exactly like "ignore".
  // GCC does the same (gcc/cp/contracts.cc, contract_semantic_emits_no_check),
  // so the two implementations agree on the observable behaviour.  Lowering it
  // to an optimiser hint instead -- llvm.assume, or the UB-preserving form the
  // implicit checks already use -- is future work, and needs a decision about
  // whether a user-written predicate is safe to feed the optimiser given it may
  // have been false all along.
  if (Semantic == Ignore || Semantic == ContractEvaluationSemantic::Assume) {
    if (ContinueBlock)
      Builder.CreateBr(ContinueBlock);
    return;
  }

  // D4405: a runtime evaluation is never prove or diagnose, which apply to
  // the analysis evaluation only (EmitContractStmtAsFullStmt and
  // DiagnoseCallerSideProvingContracts); a dynamic selection's table records
  // them as ignore.
  assert(!isProvingContractSemantic(Semantic) &&
         "a runtime evaluation with prove or diagnose");

  const auto Style = [&]() {
    switch (Semantic) {
    case Enforce: {
      if (!getLangOpts().Exceptions)
        return ContractEmissionStyle::Shared;
    }
      [[fallthrough]];
    case Observe:
    case NoexceptEnforce:
    case NoexceptObserve:
      return ContractEmissionStyle::Inline;
    case QuickEnforce:
      return ContractEmissionStyle::Shared;
    case ContractEvaluationSemantic::Ignore:
    case ContractEvaluationSemantic::Assume:
    case ContractEvaluationSemantic::Prove:
    case ContractEvaluationSemantic::Diagnose:
      llvm_unreachable("unhandled semantic");
    }
    llvm_unreachable("unhandled contract evaluation semantic");
  }();

  auto Violation = [&]() {
    switch (Style) {
    case ContractEmissionStyle::Inline:
      return createBasicBlock("contract.violation");
    case ContractEmissionStyle::Shared:
      assert(Semantic == Enforce || Semantic == QuickEnforce);
      return Semantic == Enforce
                 ? GetSharedContractViolationEnforceBlock(S.getContractKind())
                 : GetSharedContractViolationTrapBlock();
    }
    llvm_unreachable("unhandled contract emission style");
  }();

  llvm::BasicBlock *End = createBasicBlock("contract.end");

  // The violation's data block, with the basic descriptor-table pointer
  // patched in; a local handler's extended descriptor is used in the stack
  // data block instead.
  llvm::Constant *ViolationInfo =
      Semantic != ContractEvaluationSemantic::QuickEnforce
          ? BuildContractViolationInfo(*this, S)
          : nullptr;

  CurrentContractInfo CCInfo{};
  CCInfo.Contract = &S;
  CCInfo.Style = Style;
  CCInfo.Semantic = Semantic;
  CCInfo.Violation = Violation;
  CCInfo.End = End;
  CCInfo.ViolationInfoGV = ViolationInfo;
  CurrentContractRAII CurContractRAII(*this, CCInfo);

  // The predicate of a postcondition with captures reports `post' like any
  // other; `post_capture' is only for an exception from constructing or
  // destroying a capture (EmitPostconditionCaptureInit), P3098 4.6.1.
  const bool IsPostCapture = false;

  llvm::Value *BranchOn;
  // If the label's local violation handler answers an evaluation_exception by
  // rethrowing, catching the predicate's exception only to hand it to that
  // handler is pure overhead -- let it propagate instead.
  if (getLangOpts().Exceptions && getLangOpts().ContractExceptions &&
      S.predicateCanThrow() &&
      !contractLocalHandlerAlwaysRethrows(*this, S, Semantic, IsPostCapture)) {
    // Base P2900 evaluation_exception: a predicate that itself throws is a
    // violation with detection_mode ExceptionRaised.  Evaluate the predicate
    // inside a synthetic catch-all try (emitting both the guarded predicate and
    // the handler as direct IR), storing the predicate's boolean into a slot
    // pre-initialized to true so that, if it threw, the post-catch load makes
    // the assertion appear failed and falls through to the predicate-false
    // path.
    Address EHPredicateStore = CreateTempAlloca(
        Builder.getInt1Ty(), CharUnits::One(), "contract.pred.value");
    Builder.CreateStore(Builder.getTrue(), EHPredicateStore);

    assert(Builder.GetInsertBlock());
    EnsureInsertPoint();

    auto *Try = BuildContractCatchAllTry(S.getCond()->getExprLoc(), *this);
    EnterCXXTryStmt(*Try);

    // Try body: evaluate the predicate directly and record its value.
    llvm::Value *CondVal = EmitScalarExpr(S.getCond());
    if (CondVal->getType() != Builder.getInt1Ty())
      CondVal = Builder.CreateIsNotNull(CondVal, "contract.tobool");
    Builder.CreateStore(CondVal, EHPredicateStore);

    // Catch body: report the ExceptionRaised violation while the exception is
    // still live (so the handler can observe it via std::current_exception).
    ExitCXXTryStmtWithCatchIR(*Try, [&] {
      if (Semantic == Enforce || Semantic == Observe ||
          Semantic == NoexceptEnforce || Semantic == NoexceptObserve) {
        // The same block the predicate-false path builds: the label's local
        // handler and query have to be reachable from here too.
        llvm::Value *DataPtr = emitContractDataBlock(*this, S, ViolationInfo);
        EmitCxaContractViolationCall(
            S.getContractKind(), Semantic, ExceptionRaised, DataPtr,
            /*IsNoExcept=*/Semantic == NoexceptEnforce ||
                Semantic == NoexceptObserve,
            IsPostCapture);
      } else if (Semantic == QuickEnforce) {
        CreateTrap(*this);
      } else {
        llvm_unreachable("Unhandled semantic");
      }
    });

    BranchOn = Builder.CreateLoad(EHPredicateStore);
  } else {
    BranchOn = EmitScalarExpr(S.getCond());
    // In C mode, the condition may be an integer rather than i1.
    // Convert to boolean for the branch.
    if (BranchOn->getType() != Builder.getInt1Ty())
      BranchOn = Builder.CreateIsNotNull(BranchOn, "contract.tobool");
  }

  if (Style == ContractEmissionStyle::Shared && Semantic == Enforce) {
    assert(!getLangOpts().Exceptions &&
           "an enforce check shares its violation block only without "
           "exceptions");
    EnsureInsertPoint();
    ContractData->GetSharedEnforceBlock(*this, S.getContractKind())
        .IncomingPHI->addIncoming(ViolationInfo, Builder.GetInsertBlock());
  }

  // The violation arm is unlikely, as [[unlikely]] would mark it, under every
  // semantic that checks.  The enforce semantics' entry points are noreturn,
  // which already says as much; observe, noexcept_observe and quick_enforce
  // (whose trap block is shared) had no hint.
  Builder.CreateCondBr(
      BranchOn, End, Violation,
      llvm::MDBuilder(getLLVMContext()).createLikelyBranchWeights());

  // If we're creating a trap, the violation block will be created once for the
  // function. Otherwise, we need to create a call to the violation handler.
  if (Style == ContractEmissionStyle::Inline) {
    EmitBlock(CurContract()->Violation);
    Builder.SetInsertPoint(CurContract()->Violation);

    llvm::Value *DataPtr =
        emitContractDataBlock(*this, S, CurContract()->ViolationInfoGV);

    // For the _pf path, call the predicate_false entry point.
    EmitCxaContractViolationCall(S.getContractKind(), Semantic, PredicateFailed,
                                 DataPtr,
                                 /*IsNoExcept=*/Semantic == NoexceptEnforce ||
                                     Semantic == NoexceptObserve,
                                 IsPostCapture);

    // For observe semantics, emit an observable checkpoint after the
    // entry point returns, then branch to the end block.
    if (Semantic == Observe || Semantic == NoexceptObserve) {
      // The observe entry point returns. Continue to end.
      Builder.CreateBr(End);
    }
    // For enforce, the entry point is noreturn, so no branch needed.
  }

  EmitBlock(End);

  // In the P3595 dynamic dispatch, each arm must rejoin a shared continuation
  // block instead of falling through to subsequent statements.
  if (ContinueBlock)
    Builder.CreateBr(ContinueBlock);
}

//===----------------------------------------------------------------------===//
// P3097: Virtual Function Contract Wrappers
//===----------------------------------------------------------------------===//

void CodeGenFunction::EmitInterfacePreconditionsAndCaptures(
    const CXXMethodDecl *MD) {
  if (!MD->hasContracts())
    return;
  for (auto *CS : MD->getContracts()->contracts()) {
    if (CS->getContractKind() == ContractKind::Pre) {
      EmitStmt(CS);
    } else if (CS->getContractKind() == ContractKind::Post &&
               CS->hasCaptures()) {
      EmitPostconditionCaptures(CS, MD->getDeclContext());
    }
  }
}

/// Whether an exception from the evaluation of \p FD's interface contracts
/// outside its body -- in its P3097 wrapper, or at a call that checks them
/// (a C-variadic function, a virtual destructor) -- must terminate the
/// program: \p FD is noexcept, so it would in \p FD's body.
static bool interfaceContractsTerminateOnThrow(CodeGenFunction &CGF,
                                               const FunctionDecl *FD) {
  if (!CGF.getLangOpts().CXXExceptions)
    return false;
  const auto *FPT = FD->getType()->getAs<FunctionProtoType>();
  return FPT && !isUnresolvedExceptionSpec(FPT->getExceptionSpecType()) &&
         FPT->isNothrow();
}

llvm::Function *
CodeGenModule::getOrEmitVirtualContractWrapper(const CXXMethodDecl *MD) {
  auto It = VirtualContractWrappers.find(MD);
  if (It != VirtualContractWrappers.end())
    return It->second;

  // Build the wrapper function type (same as the method).
  const CGFunctionInfo &FnInfo = getTypes().arrangeCXXMethodDeclaration(MD);
  llvm::FunctionType *FnTy = getTypes().GetFunctionType(FnInfo);

  // Mangle the wrapper name.
  SmallString<256> WrapperName;
  {
    llvm::raw_svector_ostream OS(WrapperName);
    getCXXABI().getMangleContext().mangleName(GlobalDecl(MD), OS);
    OS << ".__contract_wrapper";
  }

  // Create the function with internal linkage.
  llvm::Function *WrapperFn = llvm::Function::Create(
      FnTy, llvm::GlobalValue::InternalLinkage, WrapperName, &getModule());
  WrapperFn->addFnAttr(llvm::Attribute::NoInline);

  VirtualContractWrappers[MD] = WrapperFn;

  // Emit the wrapper body.
  CodeGenFunction CGF(*this);
  CGF.EmitVirtualContractWrapperBody(WrapperFn, MD);

  return WrapperFn;
}

void CodeGenFunction::EmitVirtualDtorCallWithContracts(
    const CXXDestructorDecl *Dtor, llvm::Value *This,
    llvm::function_ref<void()> EmitCall) {
  bool AnyActive = false;
  if (getLangOpts().ContractsP3097 && Dtor->hasContracts())
    for (auto *CS : Dtor->getContracts()->contracts())
      // As for EmitCXXMemberOrOperatorMemberCallExpr's wrapper: the P4405
      // analysis evaluations need no wrapper.
      if (CS->ensureRuntimeSemantic(getContext(), Dtor->getDeclContext()) !=
          ContractEvaluationSemantic::Ignore) {
        AnyActive = true;
        break;
      }
  if (!AnyActive) {
    EmitCall();
    return;
  }
  // Checked at the call: this caller's own code, which another TU's copy of
  // it may leave out, so never settled (noteContractEvaluation).
  CurFnHoldsUnsettledContract = true;

  // As EmitCallWithCallSiteContracts, with no parameters to bind: look at
  // the call through the destructor for the state the contract emitters
  // read, and give this call site captures and selections of its own.
  SmallVector<std::pair<const Decl *, std::optional<Address>>, 4> SavedDecls;
  SmallVector<std::pair<const ContractStmt *, std::optional<Address>>, 2>
      SavedCaptureFlags, SavedSelections;
  auto SaveAndErase = [](auto &Map, auto *Key, auto &Saved) {
    auto It = Map.find(Key);
    if (It == Map.end()) {
      Saved.push_back({Key, std::nullopt});
    } else {
      Saved.push_back({Key, It->second});
      Map.erase(It);
    }
  };
  for (const ContractStmt *CS : Dtor->postconditions()) {
    if (!CS->hasCaptures())
      continue;
    for (const Decl *D : CS->getCapturesDeclStmt()->decls())
      SaveAndErase(LocalDeclMap, D, SavedDecls);
    SaveAndErase(ContractData->CaptureInitFailed, CS, SavedCaptureFlags);
    SaveAndErase(ContractData->DynSelection, CS, SavedSelections);
  }

  const Decl *SavedCodeDecl = CurCodeDecl;
  const Decl *SavedFuncDecl = CurFuncDecl;
  llvm::Value *SavedThis = CXXThisValue;
  CharUnits SavedThisAlign = CXXThisAlignment;
  bool SavedInterfaceEvaluation = InContractInterfaceEvaluation;
  auto EnterDtor = [&] {
    CurCodeDecl = Dtor;
    CurFuncDecl = Dtor;
    CXXThisValue = This;
    CXXThisAlignment = CharUnits();
    InContractInterfaceEvaluation = true;
  };
  auto LeaveDtor = [&] {
    CurCodeDecl = SavedCodeDecl;
    CurFuncDecl = SavedFuncDecl;
    CXXThisValue = SavedThis;
    CXXThisAlignment = SavedThisAlign;
    InContractInterfaceEvaluation = SavedInterfaceEvaluation;
  };

  // For a noexcept destructor, as in its body, an exception from the
  // evaluation terminates (interfaceContractsTerminateOnThrow).
  bool TerminateOnThrow = interfaceContractsTerminateOnThrow(*this, Dtor);
  if (TerminateOnThrow)
    EHStack.pushTerminate();
  RunCleanupsScope CaptureScope(*this);
  EnterDtor();
  EmitInterfacePreconditionsAndCaptures(Dtor);
  LeaveDtor();
  EmitCall();
  if (HaveInsertPoint()) {
    EnterDtor();
    EmitPostContracts(nullptr);
    LeaveDtor();
  }
  CaptureScope.ForceCleanup();
  if (TerminateOnThrow)
    EHStack.popTerminate();

  for (auto &[CS, Flag] : SavedCaptureFlags) {
    ContractData->CaptureInitFailed.erase(CS);
    if (Flag)
      ContractData->CaptureInitFailed.insert({CS, *Flag});
  }
  for (auto &[CS, Sel] : SavedSelections) {
    ContractData->DynSelection.erase(CS);
    if (Sel)
      ContractData->DynSelection.insert({CS, *Sel});
  }
  for (auto &[D, Addr] : llvm::reverse(SavedDecls)) {
    LocalDeclMap.erase(D);
    if (Addr)
      LocalDeclMap.insert({D, *Addr});
  }
}

RValue CodeGenFunction::EmitCallWithCallSiteContracts(
    const CXXMethodDecl *MD, const CGFunctionInfo &FnInfo,
    const CGCallee &Callee, ReturnValueSlot Slot, CallArgList &Args,
    llvm::Value *ContractThis, const CallExpr *CE,
    llvm::CallBase **CallOrInvoke) {
  // The contracts name MD's parameters, and are emitted here as they would
  // be in MD's wrapper: bind each parameter to the argument evaluated for
  // it, and look at the call through MD for the rest of the state the
  // contract emitters read.  Entries this displaces are put back at the end.
  SmallVector<std::pair<const Decl *, std::optional<Address>>, 8> SavedDecls;
  auto Unbind = [&](const Decl *D) {
    auto It = LocalDeclMap.find(D);
    if (It == LocalDeclMap.end()) {
      SavedDecls.push_back({D, std::nullopt});
      return;
    }
    SavedDecls.push_back({D, It->second});
    LocalDeclMap.erase(It);
  };

  // A scalar argument is copied to a temporary the predicates can name, and
  // passed on from there, so the call sees what the preconditions saw.
  struct Reload {
    CallArg *Arg;
    Address Temp;
    QualType Ty;
  };
  SmallVector<Reload, 4> Reloads;
  unsigned ArgIdx = 1; // Args[0] is the object argument.
  for (const ParmVarDecl *PD : MD->parameters()) {
    CallArg &A = Args[ArgIdx++];
    Unbind(PD);
    if (A.hasLValue()) {
      RValue RV = A.getRValue(*this);
      A = CallArg(RV, A.Ty);
    }
    RValue RV = A.getKnownRValue();
    Address Addr = Address::invalid();
    if (RV.isAggregate()) {
      Addr = RV.getAggregateAddress().withElementType(
          ConvertTypeForMem(PD->getType()));
    } else {
      Addr = CreateMemTemp(PD->getType(), "contract.arg");
      LValue LV = MakeAddrLValue(Addr, PD->getType());
      if (RV.isComplex())
        EmitStoreOfComplex(RV.getComplexVal(), LV, /*isInit=*/true);
      else
        EmitStoreOfScalar(RV.getScalarVal(), LV, /*isInit=*/true);
      Reloads.push_back({&A, Addr, PD->getType()});
    }
    LocalDeclMap.insert({PD, Addr});
  }

  // Every call site makes its own captures, and its own capture-failure
  // flags.
  SmallVector<std::pair<const ContractStmt *, std::optional<Address>>, 2>
      SavedCaptureFlags, SavedSelections;
  auto SaveAndErase = [](auto &Map, const ContractStmt *CS, auto &Saved) {
    auto It = Map.find(CS);
    if (It == Map.end()) {
      Saved.push_back({CS, std::nullopt});
    } else {
      Saved.push_back({CS, It->second});
      Map.erase(It);
    }
  };
  for (const ContractStmt *CS : MD->postconditions()) {
    if (!CS->hasCaptures())
      continue;
    for (const Decl *D : CS->getCapturesDeclStmt()->decls())
      Unbind(D);
    SaveAndErase(ContractData->CaptureInitFailed, CS, SavedCaptureFlags);
    SaveAndErase(ContractData->DynSelection, CS, SavedSelections);
  }

  // The state the contract emitters read: the function whose contracts
  // these are, and its object.
  const Decl *SavedCodeDecl = CurCodeDecl;
  const Decl *SavedFuncDecl = CurFuncDecl;
  llvm::Value *SavedThis = CXXThisValue;
  CharUnits SavedThisAlign = CXXThisAlignment;
  bool SavedInterfaceEvaluation = InContractInterfaceEvaluation;
  auto EnterMD = [&] {
    CurCodeDecl = MD;
    CurFuncDecl = MD;
    CXXThisValue = ContractThis;
    CXXThisAlignment = CharUnits();
    InContractInterfaceEvaluation = true;
  };
  auto LeaveMD = [&] {
    CurCodeDecl = SavedCodeDecl;
    CurFuncDecl = SavedFuncDecl;
    CXXThisValue = SavedThis;
    CXXThisAlignment = SavedThisAlign;
    InContractInterfaceEvaluation = SavedInterfaceEvaluation;
  };

  // For a noexcept MD, as in its body, an exception from the evaluation
  // terminates (interfaceContractsTerminateOnThrow).
  bool TerminateOnThrow = interfaceContractsTerminateOnThrow(*this, MD);
  if (TerminateOnThrow)
    EHStack.pushTerminate();

  // The captures are destroyed after the postconditions, before the rest of
  // the full-expression.
  RunCleanupsScope CaptureScope(*this);

  EnterMD();
  EmitInterfacePreconditionsAndCaptures(MD);
  LeaveMD();

  for (const Reload &R : Reloads) {
    LValue LV = MakeAddrLValue(R.Temp, R.Ty);
    R.Arg->setRValue(R.Ty->isAnyComplexType()
                         ? RValue::getComplex(
                               EmitLoadOfComplex(LV, CE->getExprLoc()))
                         : RValue::get(EmitLoadOfScalar(LV, CE->getExprLoc())));
  }

  QualType ResultTy = MD->getReturnType();
  ReturnValueSlot CallSlot = Slot;
  if (hasAggregateEvaluationKind(ResultTy) && Slot.isNull())
    CallSlot = ReturnValueSlot(CreateMemTemp(ResultTy, "contract.result"),
                               /*IsVolatile=*/false);
  RValue RV = EmitCall(FnInfo, Callee, CallSlot, Args, CallOrInvoke,
                       /*IsMustTail=*/false, CE->getExprLoc());

  llvm::Value *ScalarResult = nullptr;
  if (HaveInsertPoint()) {
    Address SavedReturnValue = ReturnValue;
    bool SavedBound = ContractResultBoundToReturnSlot;
    EnterMD();
    if (ResultTy->isVoidType()) {
      EmitPostContracts(nullptr);
    } else if (RV.isScalar()) {
      ReturnValue = CreateIRTempWithoutCast(ResultTy, "contract.retval");
      ScalarResult = EmitPostContracts(RV.getScalarVal());
    } else if (RV.isComplex()) {
      ReturnValue = CreateMemTemp(ResultTy, "contract.retval");
      EmitStoreOfComplex(RV.getComplexVal(),
                         MakeAddrLValue(ReturnValue, ResultTy),
                         /*isInit=*/true);
      EmitPostContracts(nullptr);
      RV = RValue::getComplex(EmitLoadOfComplex(
          MakeAddrLValue(ReturnValue, ResultTy), CE->getExprLoc()));
    } else {
      // The call has initialized the result, and nothing owns it yet: a
      // handler that throws must destroy it ([except.ctor]/2).
      ReturnValue = RV.getAggregateAddress();
      RunCleanupsScope ResultScope(*this);
      if (getLangOpts().Exceptions)
        if (QualType::DestructionKind DK = ResultTy.isDestructedType())
          pushDestroy(EHCleanup, ReturnValue, ResultTy, getDestroyer(DK),
                      /*useEHCleanupForArray=*/true);
      EmitPostContracts(nullptr);
      ResultScope.ForceCleanup();
    }
    LeaveMD();
    ReturnValue = SavedReturnValue;
    ContractResultBoundToReturnSlot = SavedBound;
  }

  if (ScalarResult)
    CaptureScope.ForceCleanup({&ScalarResult});
  else
    CaptureScope.ForceCleanup();
  if (TerminateOnThrow)
    EHStack.popTerminate();
  if (ScalarResult)
    RV = RValue::get(ScalarResult);

  for (auto &[CS, Flag] : SavedCaptureFlags) {
    ContractData->CaptureInitFailed.erase(CS);
    if (Flag)
      ContractData->CaptureInitFailed.insert({CS, *Flag});
  }
  for (auto &[CS, Sel] : SavedSelections) {
    ContractData->DynSelection.erase(CS);
    if (Sel)
      ContractData->DynSelection.insert({CS, *Sel});
  }
  for (auto &[D, Addr] : llvm::reverse(SavedDecls)) {
    LocalDeclMap.erase(D);
    if (Addr)
      LocalDeclMap.insert({D, *Addr});
  }
  return RV;
}

void CodeGenFunction::EmitVirtualContractWrapperBody(llvm::Function *Fn,
                                                     const CXXMethodDecl *MD) {
  GlobalDecl GD(MD);
  const FunctionProtoType *FPT = MD->getType()->castAs<FunctionProtoType>();
  QualType ResultType = FPT->getReturnType();

  // Set up the function like a thunk.
  const CGFunctionInfo &FnInfo = CGM.getTypes().arrangeCXXMethodDeclaration(MD);
  CurGD = GD;
  CurFuncIsThunk = true;

  // Build FunctionArgs.
  FunctionArgList FunctionArgs;
  CGM.getCXXABI().buildThisParam(*this, FunctionArgs);
  FunctionArgs.append(MD->param_begin(), MD->param_end());

  // Start the function.
  auto NL = ApplyDebugLocation::CreateEmpty(*this);
  StartFunction(GlobalDecl(), ResultType, Fn, FnInfo, FunctionArgs,
                MD->getLocation());
  auto AL = ApplyDebugLocation::CreateArtificial(*this);

  // Set up the this pointer and CurCodeDecl so contract emission works.
  CGM.getCXXABI().EmitInstanceFunctionProlog(*this);
  CXXThisValue = CXXABIThisValue;
  CurCodeDecl = MD;
  CurFuncDecl = MD;
  // D4405: the wrapper evaluates MD's contracts, but not their analysis
  // evaluations, which are at the call (preconditions) and in MD's body.
  InContractInterfaceEvaluation = true;

  // The interface contracts are evaluated as in MD's body: for a noexcept
  // MD, an exception from a violation handler, the selector, or a capture
  // terminates the program rather than leave the wrapper (the vtable call is
  // to a noexcept function in any case).  StartFunction had no declaration
  // to take the exception specification from, so enter it here, above the
  // prologue's cleanups, and leave it below after popping the body's.
  bool TerminateOnThrow = interfaceContractsTerminateOnThrow(*this, MD);
  if (TerminateOnThrow)
    EHStack.pushTerminate();
  EHScopeStack::stable_iterator BodyDepth = EHStack.stable_begin();

  // Phase 1: Emit interface preconditions and initialize postcondition
  // captures in lexical order (same as normal function prologue).
  EmitInterfacePreconditionsAndCaptures(MD);

  // Phase 2: Virtual dispatch through the vtable.
  // Build call arguments: this + parameters.
  CallArgList CallArgs;
  QualType ThisType = MD->getThisType();
  CallArgs.add(RValue::get(LoadCXXThis()), ThisType);
  for (const ParmVarDecl *PD : MD->parameters())
    EmitDelegateCallArg(CallArgs, PD, SourceLocation());

  // Use virtual call mechanism.
  llvm::FunctionType *CallFnTy = CGM.getTypes().GetFunctionType(FnInfo);
  Address ThisAddr = LoadCXXThisAddress();
  CGCallee Callee = CGCallee::forVirtual(nullptr, GD, ThisAddr, CallFnTy);

  // Determine return value slot.
  ReturnValueSlot Slot;
  if (!ResultType->isVoidType() &&
      (FnInfo.getReturnInfo().getKind() == ABIArgInfo::Indirect ||
       hasAggregateEvaluationKind(ResultType)))
    Slot = ReturnValueSlot(ReturnValue, ResultType.isVolatileQualified(),
                           /*IsUnused=*/false, /*IsExternallyDestructed=*/true);

  // Emit the virtual call.
  llvm::CallBase *CallOrInvoke = nullptr;
  RValue RV = EmitCall(FnInfo, Callee, Slot, CallArgs, &CallOrInvoke);

  // Phase 3: Emit interface postconditions using the standard path.
  llvm::Value *RetVal = (!ResultType->isVoidType() && Slot.isNull())
                            ? RV.getScalarVal()
                            : nullptr;
  // The call has initialized the returned object, which a handler that
  // throws must destroy ([except.ctor]/2).  A thunk gets no live flag from
  // StartFunction, so make one here, set now that the object exists.
  if (getLangOpts().Exceptions && !Slot.isNull())
    if (const auto *RD = ResultType->getAsCXXRecordDecl();
        RD && RD->hasDefinition() && !RD->hasTrivialDestructor()) {
      RawAddress Flag = CreateTempAlloca(Builder.getInt1Ty(), CharUnits::One(),
                                         "retval.live");
      Builder.CreateStore(Builder.getTrue(), Flag);
      ReturnValueLiveFlag = Flag.getPointer();
    }
  // A predicate may mutate through the result binding, which names the return
  // slot, so take back whatever EmitPostContracts leaves there -- the same
  // reason the ordinary epilogue does.
  if (llvm::Value *NewRetVal = EmitPostContractsWithRetvalCleanup(RetVal);
      NewRetVal && NewRetVal != RetVal)
    RV = RValue::get(NewRetVal);

  // Emit return via the thunk pattern.
  if (!ResultType->isVoidType() && Slot.isNull())
    CGM.getCXXABI().EmitReturnFromThunk(*this, RV, ResultType);

  // Disable tail call / autorelease.
  AutoreleaseResult = false;

  // Leave the noexcept scope, which FinishFunction, seeing no declaration,
  // would not.
  if (TerminateOnThrow) {
    PopCleanupBlocks(BodyDepth);
    EHStack.popTerminate();
  }

  // Clear thunk state and finish.
  CurCodeDecl = nullptr;
  CurFuncDecl = nullptr;
  FinishFunction();
}

/// P3400: whether every translation unit's copy of the function holding
/// \p S makes the same choice as far as throwing goes, from the label's
/// allowed set (the same in every TU): a non-empty subset of
/// {noexcept_enforce, quick_enforce, assume}, which never throw from the
/// evaluation; or {enforce} alone, the same handler call in every copy; or,
/// in a noexcept function \p FD, a subset of {enforce, noexcept_enforce,
/// quick_enforce, assume}, where a throwing handler terminates in every copy.
/// A dynamically selected evaluation is never settled: under such a label
/// it keeps to the allowed set and throws nothing from the check (an invalid
/// selection is a noexcept_enforce violation or a quick_enforce trap,
/// DECISIONS.md E137 and E138), but the copy calls the selector, an ordinary
/// external function with no exception specification that another copy need
/// not call, so an exception from it, and what it reads and writes, is the
/// caller's to see.  (gnu_gcc's contract_evaluation_settled_p.)  One residual
/// is accepted, as there (E137): a handler on its way to termination may
/// observe memory a caller optimized with a quick_enforce or assume copy's
/// facts.
static bool isContractEvaluationSettled(const ContractStmt &S,
                                        const Decl *FD) {
  using CES = ContractEvaluationSemantic;
  if (S.isDynamic())
    return false;
  auto Bit = [](CES Sem) { return 1u << static_cast<unsigned>(Sem); };
  unsigned Mask = S.getAllowedMask();
  unsigned Continuing = Bit(CES::Ignore) | Bit(CES::Observe) |
                        Bit(CES::NoexceptObserve) | Bit(CES::Prove) |
                        Bit(CES::Diagnose);
  if (!Mask || (Mask & Continuing))
    return false;
  if (!(Mask & Bit(CES::Enforce)) || Mask == Bit(CES::Enforce))
    return true;
  const auto *F = dyn_cast_or_null<FunctionDecl>(FD);
  const auto *FPT = F ? F->getType()->getAs<FunctionProtoType>() : nullptr;
  return FPT && FPT->isNothrow();
}

/// Refinable bodies: note an evaluation of \p S in the function being
/// emitted.  Unless its semantic is settled, another translation unit's copy
/// of the function may evaluate it differently, so facts about this copy's
/// body may not reach callers that may run another copy (GenerateCode).
/// P3100's implicit checks are never noted: going past one is undefined
/// behaviour in every copy.
void CodeGenFunction::noteContractEvaluation(const ContractStmt &S) {
  // P3400: a settled evaluation leaves the same choice in every copy.
  if (isContractEvaluationSettled(S, CurFuncDecl))
    return;
  CurFnHoldsUnsettledContract = true;
}
