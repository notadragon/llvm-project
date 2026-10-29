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

// Emit the contract expression.
void CodeGenFunction::EmitContractStmt(const ContractStmt &S) {
  EmitContractStmtAsFullStmt(S);
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

void CodeGenFunction::emitCheckForSemantic(const ContractStmt &S,
                                           ContractEvaluationSemantic Semantic,
                                           llvm::BasicBlock *ContinueBlock) {
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

/// Refinable bodies: note an evaluation of \p S in the function being
/// emitted.  Unless its semantic is settled, another translation unit's copy
/// of the function may evaluate it differently, so facts about this copy's
/// body may not reach callers that may run another copy (GenerateCode).
/// P3100's implicit checks are never noted: going past one is undefined
/// behaviour in every copy.
void CodeGenFunction::noteContractEvaluation(const ContractStmt &S) {
  CurFnHoldsUnsettledContract = true;
}
