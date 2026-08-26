//===--- CGContracts.cpp - Emit LLVM Code for C++ contracts -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit blocks.
//
//===----------------------------------------------------------------------===//

#include "CGContracts.h"
#include "CGCXXABI.h"
#include "CGCall.h"
#include "CGDebugInfo.h"
#include "CGObjCRuntime.h"
#include "CGOpenCLRuntime.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "ConstantEmitter.h"
#include "TargetInfo.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/StmtCXX.h"
#include "clang/CodeGen/ConstantInitBuilder.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ScopedPrinter.h"
#include <algorithm>
#include <cstdio>
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
constexpr ContractDetectionMode ExceptionRaised =
    ContractDetectionMode::ExceptionRaised;

namespace clang::CodeGen {

enum ContractEmissionStyle {
  /// Emit the contract violation as an inline basic block immediately following
  /// the predicate. The basic block is not shared by other contracts.
  Inline,

  /// Emit a single shared contract violation handler per-function.
  /// This only works when exceptions are disabled, otherwise the violation
  /// handler
  /// may throw from the violation handler.
  Shared
};

template <class T>
static llvm::Constant *CreateConstantInt(CodeGenFunction &CGF, T Sem) {
  static_assert(std::is_same_v<T, ContractEvaluationSemantic> ||
                std::is_same_v<T, ContractDetectionMode>);
  return llvm::ConstantInt::get(CGF.IntTy, (int)Sem);
}

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
        /*IsNoExcept=*/false, /*HasLocalHandler=*/false);
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
  // One shared enforce block per assertion kind (Pre/Post/Assert/Implicit),
  // indexed by the ContractKind value, so the enforced-violation handler is
  // reported with the contract's real kind rather than a single hardcoded kind.
  std::optional<SharedEnforceBlock> EnforceBlocks[4];
  std::optional<SharedTrapBlock> TrapBlock;
  std::optional<CurrentContractInfo> CurContract;

  // P3098: per-postcondition i1 flag, true once that postcondition's capture
  // construction has been observed to throw (observe semantic only -- see
  // EmitPostconditionCaptureInit's catch body). Consulted by EmitPostContracts
  // to skip that postcondition's predicate. Only populated for postconditions
  // whose capture initializers can actually throw; absence means "never
  // fails" (EmitPostContracts pull for LoadPostconditionCaptureFailed).
  llvm::DenseMap<const ContractStmt *, Address> CaptureInitFailed;

  SharedTrapBlock &GetSharedTrapBlock(CodeGenFunction &CGF) {
    if (!TrapBlock)
      TrapBlock = SharedTrapBlock::Create(CGF);
    return *TrapBlock;
  }

  SharedEnforceBlock &GetSharedEnforceBlock(CodeGenFunction &CGF,
                                            ContractKind Kind) {
    auto Idx = static_cast<unsigned>(Kind);
    assert(Idx < 4 && "unexpected contract kind");
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

struct CurrentContractRAII {
  CurrentContractRAII(CodeGenFunction &CGF, CurrentContractInfo CurContract)
      : CGF(CGF) {
    assert(!CGF.ContractData->CurContract);
    CGF.ContractData->CurContract.emplace(std::move(CurContract));
  }
  ~CurrentContractRAII() {
    assert(CGF.ContractData->CurContract);
    CGF.ContractData->CurContract.reset();
  }
  CodeGenFunction &CGF;
};

CGContractData *CGContractDataDeleter::Create() { return new CGContractData(); }
void CGContractDataDeleter::operator()(CGContractData *Data) const {

  if (Data)
    delete Data;
}

llvm::BasicBlock *
CodeGenFunction::GetSharedContractViolationEnforceBlock(ContractKind Kind,
                                                        bool Create) {
  auto Idx = static_cast<unsigned>(Kind);
  assert(Idx < 3 && "unexpected contract kind");
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

  const char *ModeStr;
  switch (Mode) {
  case PredicateFailed:
    ModeStr = "pf";
    break;
  case ExceptionRaised:
    ModeStr = "ex";
    break;
  default:
    llvm_unreachable("bad detection mode for cxa entry point");
  }

  char Buf[128];
  if (IsNoExcept)
    std::snprintf(Buf, sizeof(Buf),
                  "__cxa_contract_violation_%s_%s_%s_noexcept", KindStr, SemStr,
                  ModeStr);
  else
    std::snprintf(Buf, sizeof(Buf), "__cxa_contract_violation_%s_%s_%s",
                  KindStr, SemStr, ModeStr);
  return std::string(Buf);
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

  if (auto *Existing = CGM.getModule().getGlobalVariable(Name))
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

// Check if function can throw based on prototype noexcept, also works for
// destructors which are implicitly noexcept but can be marked noexcept(false).
static bool FunctionCanThrow(const FunctionDecl *D) {
  const auto *Proto = D->getType()->getAs<FunctionProtoType>();
  if (!Proto) {
    // Function proto is not found, we conservatively assume throwing.
    return true;
  }
  // canThrow() already accounts for every exception-specification form,
  // including the dynamic ones, so it is the whole test.
  return Proto->canThrow() != CT_Cannot;
}

static bool StmtCanThrow(const Stmt *S) {
  if (const auto *CE = dyn_cast<CallExpr>(S)) {
    const auto *Callee = CE->getDirectCallee();
    if (!Callee)
      // We don't have direct callee. Conservatively assume throwing.
      return true;

    if (FunctionCanThrow(Callee))
      return true;

    // Fall through to visit the children.
  }

  if (isa<CXXThrowExpr>(S))
    return true;

  if (const auto *TE = dyn_cast<CXXBindTemporaryExpr>(S)) {
    // Special handling of CXXBindTemporaryExpr here as calling of Dtor of the
    // temporary is not part of `children()` as covered in the fall through.
    // We need to mark entire statement as throwing if the destructor of the
    // temporary throws.
    const auto *Dtor = TE->getTemporary()->getDestructor();
    if (FunctionCanThrow(Dtor))
      return true;

    // Fall through to visit the children.
  }

  for (const auto *child : S->children())
    if (StmtCanThrow(child))
      return true;

  return false;
}

/// The local violation handler LabelRD carries, or null if it has none.  The
/// rethrow analysis below has to reason about exactly the method the
/// trampoline will call, so both go through here.
static const CXXMethodDecl *
findLocalViolationHandler(const CXXRecordDecl *LabelRD) {
  for (const auto *M : LabelRD->methods())
    if (M->getDeclName().isIdentifier() &&
        M->getDeclName().getAsIdentifierInfo()->getName() ==
            "handle_contract_violation")
      return M;
  return nullptr;
}

static llvm::Function *
getOrCreateLocalHandlerTrampoline(CodeGenModule &CGM,
                                  const CXXRecordDecl *LabelRD) {
  const CXXMethodDecl *HCVMethod = findLocalViolationHandler(LabelRD);
  if (!HCVMethod)
    return nullptr;

  std::string TrampolineName = ("__clang_contract_local_handler_" +
                                CGM.getMangledName(GlobalDecl(HCVMethod)))
                                   .str();

  if (auto *Existing = CGM.getModule().getFunction(TrampolineName))
    return Existing;

  llvm::LLVMContext &LLVMCtx = CGM.getLLVMContext();
  llvm::Type *Int32Ty = llvm::Type::getInt32Ty(LLVMCtx);
  llvm::Type *PtrTy = llvm::PointerType::getUnqual(LLVMCtx);

  // New ABI: trampoline signature is int(const void*, const void*)
  // Second arg is pointer to contract_violation (passed as void*).
  llvm::FunctionType *TrampolineFTy =
      llvm::FunctionType::get(Int32Ty, {PtrTy, PtrTy}, false);

  llvm::Function *TrampolineFn =
      llvm::Function::Create(TrampolineFTy, llvm::GlobalValue::InternalLinkage,
                             TrampolineName, &CGM.getModule());

  llvm::BasicBlock *Entry =
      llvm::BasicBlock::Create(LLVMCtx, "entry", TrampolineFn);
  llvm::IRBuilder<> B(Entry);

  llvm::Value *LabelPtrArg = TrampolineFn->getArg(0);
  llvm::Value *CvPtrArg = TrampolineFn->getArg(1);

  // Cast const void* to const contract_violation& for the method call.
  // The second arg is already a pointer to contract_violation.
  llvm::Value *ViolationArg = CvPtrArg;

  llvm::Constant *MethodAddr = CGM.GetAddrOfFunction(GlobalDecl(HCVMethod));
  llvm::FunctionType *MethodFTy = CGM.getTypes().GetFunctionType(
      CGM.getTypes().arrangeCXXMethodDeclaration(HCVMethod));

  bool ReturnsVoid = HCVMethod->getReturnType()->isVoidType();
  bool IsStatic = HCVMethod->isStatic();

  SmallVector<llvm::Value *, 2> CallArgs;
  if (!IsStatic)
    CallArgs.push_back(LabelPtrArg);
  CallArgs.push_back(ViolationArg);

  llvm::Value *CallResult = B.CreateCall(MethodFTy, MethodAddr, CallArgs);

  if (ReturnsVoid) {
    B.CreateRet(llvm::ConstantInt::get(Int32Ty, 0));
  } else {
    llvm::Value *IntResult = B.CreateIntCast(CallResult, Int32Ty, true);
    B.CreateRet(IntResult);
  }

  return TrampolineFn;
}

static llvm::Function *
getOrCreateQueryTrampoline(CodeGenModule &CGM, const CXXRecordDecl *LabelRD) {
  const CXXMethodDecl *QueryMethod = nullptr;
  for (const auto *M : LabelRD->methods()) {
    if (M->getDeclName().isIdentifier() &&
        M->getDeclName().getAsIdentifierInfo()->getName() == "query") {
      QueryMethod = M;
      break;
    }
  }
  if (!QueryMethod)
    return nullptr;

  std::string TrampolineName =
      ("__clang_contract_query_" + CGM.getMangledName(GlobalDecl(QueryMethod)))
          .str();

  if (auto *Existing = CGM.getModule().getFunction(TrampolineName))
    return Existing;

  llvm::LLVMContext &LLVMCtx = CGM.getLLVMContext();
  llvm::Type *PtrTy = llvm::PointerType::getUnqual(LLVMCtx);
  llvm::Type *SizeTy = CGM.SizeTy;

  // Signature: void*(const void* label_ptr, const void* key, size_t index)
  llvm::FunctionType *TrampolineFTy =
      llvm::FunctionType::get(PtrTy, {PtrTy, PtrTy, SizeTy}, false);

  llvm::Function *TrampolineFn =
      llvm::Function::Create(TrampolineFTy, llvm::GlobalValue::InternalLinkage,
                             TrampolineName, &CGM.getModule());

  llvm::BasicBlock *Entry =
      llvm::BasicBlock::Create(LLVMCtx, "entry", TrampolineFn);
  llvm::IRBuilder<> B(Entry);

  llvm::Value *LabelPtrArg = TrampolineFn->getArg(0);
  llvm::Value *KeyArg = TrampolineFn->getArg(1);
  llvm::Value *IndexArg = TrampolineFn->getArg(2);

  llvm::Constant *MethodAddr = CGM.GetAddrOfFunction(GlobalDecl(QueryMethod));
  llvm::FunctionType *MethodFTy = CGM.getTypes().GetFunctionType(
      CGM.getTypes().arrangeCXXMethodDeclaration(QueryMethod));

  bool IsStatic = QueryMethod->isStatic();

  SmallVector<llvm::Value *, 3> CallArgs;
  if (!IsStatic)
    CallArgs.push_back(LabelPtrArg);
  CallArgs.push_back(KeyArg);
  CallArgs.push_back(IndexArg);

  llvm::Value *CallResult = B.CreateCall(MethodFTy, MethodAddr, CallArgs);
  B.CreateRet(CallResult);

  return TrampolineFn;
}

// Emit the contract expression.
void CodeGenFunction::EmitContractStmt(const ContractStmt &S) {
  assert(
      CurContract() == nullptr &&
      "contract emission is not re-entrant; there is no dispatch checkpoint");
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
// the descriptor-table pointer patched in.  This mirrors the inline
// construction in emitCheckForSemantic below; it is factored out so the dynamic
// dispatch's enforced-violation path can reuse it without re-running a
// predicate check.
static llvm::Constant *
FinishViolationInfo(CodeGenFunction &CGF,
                    UnnamedGlobalConstantDecl *ViolationGV) {
  CodeGenModule &CGM = CGF.CGM;
  llvm::Constant *DataGV =
      CGM.GetAddrOfUnnamedGlobalConstantDecl(ViolationGV, "contract.loc")
          .getPointer();

  llvm::Constant *DescTable =
      getOrCreateDescriptorTable(CGM, /*HasLocalHandler=*/false);
  if (auto *OrigGV = dyn_cast<llvm::GlobalVariable>(DataGV)) {
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
// noexcept variants when -fcontracts-p4298 is in effect.
static ContractEvaluationSemantic
resolveImplicitContractSemantic(CodeGenModule &CGM, StringRef Group,
                                const DeclContext *FnContext,
                                SourceLocation Loc) {
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
  unsigned Mask = AllContractSemanticsMask | (1u << unsigned(CES::Assume));
  if (CGM.getLangOpts().ContractsP4298)
    Mask |= (1u << unsigned(CES::NoexceptEnforce)) |
            (1u << unsigned(CES::NoexceptObserve));
  Q.AllowedMask = Mask;
  return CGM.getLangOpts().ContractOpts.resolveContractSemantic(Q);
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
      *this, "ub:class.abstract.pure.virtual", RD, RD->getLocation());

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
  // ub:expr.mul.div.by.zero, ub:expr.shift.neg.and.width).  See
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
    Builder.CreateUnreachable();
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
                                                 SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic for ub:expr.unary.dereference.nullptr at this site.
  // See EmitImplicitFlowOffReaction for the allowed-set rationale.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:expr.unary.dereference.nullptr", FD, Loc);

  // assume / ignore: `*p` is an lvalue with no defined substitute, so leave the
  // raw dereference untouched (byte-identical to no P3100).
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

bool CodeGenFunction::EmitImplicitMisalignedGuard(llvm::Value *Ptr,
                                                  llvm::Align Align,
                                                  SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic for ub:basic.align.object.alignment at this site.
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:basic.align.object.alignment", FD, Loc);

  // assume / ignore: a misaligned access is an lvalue with no defined
  // substitute, so leave the raw access untouched (byte-identical to no P3100).
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
    // id; see p3100-check-table.md.
    std::string GroupStr = HasSideEffects ? "ub:dcl.attr.assume.false.nonpure"
                                          : "ub:dcl.attr.assume.false.pure";
    ContractQuery Q;
    Q.Kind = ContractKind::Implicit;
    Q.CallerSide = false;
    Q.InConstantEvaluation = false;
    Q.Groups = ArrayRef<std::string>(GroupStr);
    Q.FnContext = FD;
    Q.Loc = Assumption->getExprLoc();
    Q.SM = &getContext().getSourceManager();
    unsigned Mask;
    if (HasSideEffects)
      Mask = (1u << unsigned(CES::Ignore)) | (1u << unsigned(CES::Assume));
    else {
      Mask = AllContractSemanticsMask | (1u << unsigned(CES::Assume));
      if (getLangOpts().ContractsP4298)
        Mask |= (1u << unsigned(CES::NoexceptEnforce)) |
                (1u << unsigned(CES::NoexceptObserve));
    }
    Q.AllowedMask = Mask;

    CES Sem = getLangOpts().ContractOpts.resolveContractSemantic(Q);

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

  // Resolve the semantic for ub:expr.expr.eval.signed.integer at this site.
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

  // assume: keep the no-overflow assumption (nsw) -- byte-identical to
  // no-P3100.
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
    Builder.CreateUnreachable();
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
  // the -fsanitize=bool/enum detection in EmitScalarRangeCheck.  When that
  // sanitizer is active for this type it takes precedence (the caller then runs
  // the sanitizer check via maybeAttachRangeForLoad).
  bool IsBool = Ty->hasBooleanRepresentation() && !Ty->isVectorType();
  bool IsEnum = Ty->isEnumeralType();
  if (!IsBool && !IsEnum)
    return Loaded;
  if ((IsBool && SanOpts.has(SanitizerKind::Bool)) ||
      (IsEnum && SanOpts.has(SanitizerKind::Enum)))
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
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:conv.lval.valid.representation.bool.enum", FD, Loc);
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
    Builder.CreateUnreachable();
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

llvm::Value *CodeGenFunction::EmitImplicitArrayBoundsGuard(llvm::Value *Idx,
                                                           QualType IdxTy,
                                                           llvm::Value *Bound,
                                                           bool Accessed,
                                                           SourceLocation Loc) {
  using CES = ContractEvaluationSemantic;

  // Resolve the semantic once for this subscript; assume leaves the raw index
  // (byte-identical -- no predicate emitted).
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  CES Sem = resolveImplicitContractSemantic(
      CGM, "ub:expr.add.out.of.bounds.known", FD, Loc);
  if (Sem == CES::Assume)
    return Idx;

  // out_of_range predicate, mirroring EmitBoundsCheckImpl: widen the index and
  // the (non-negative constant) bound to a common type and compare unsigned so
  // a negative index is caught too.  For an access the valid range is
  // [0,bound); for a one-past address (!Accessed) index == bound is allowed.
  bool IdxSigned = IdxTy->isSignedIntegerOrEnumerationType();
  unsigned IdxBits = cast<llvm::IntegerType>(Idx->getType())->getBitWidth();
  unsigned BoundBits = cast<llvm::IntegerType>(Bound->getType())->getBitWidth();
  llvm::Type *Ty = IdxBits >= BoundBits ? Idx->getType() : Bound->getType();
  llvm::Value *IdxW = Builder.CreateIntCast(Idx, Ty, IdxSigned);
  llvm::Value *BoundW = Builder.CreateIntCast(Bound, Ty, /*isSigned=*/false);
  llvm::Value *IsViolation = Accessed
                                 ? Builder.CreateICmpUGE(IdxW, BoundW, "oob")
                                 : Builder.CreateICmpUGT(IdxW, BoundW, "oob");

  // value = IsViolation ? 0 : Idx, computed unconditionally (dominates the
  // continuation), redirecting an out-of-range subscript to the valid index 0.
  llvm::Value *Zero = llvm::Constant::getNullValue(Idx->getType());
  llvm::Value *Result = Builder.CreateSelect(IsViolation, Zero, Idx, "idx.ok");
  if (Sem == CES::Ignore)
    return Result;

  llvm::BasicBlock *ViolBB = createBasicBlock("oob.viol");
  llvm::BasicBlock *ContBB = createBasicBlock("oob.ok");
  Builder.CreateCondBr(IsViolation, ViolBB, ContBB);

  EmitBlock(ViolBB);
  if (Sem == CES::QuickEnforce) {
    CreateTrap(*this);
    Builder.CreateUnreachable();
  } else {
    bool IsNoExcept =
        (Sem == CES::NoexceptEnforce || Sem == CES::NoexceptObserve);
    bool IsEnforce = (Sem == CES::Enforce || Sem == CES::NoexceptEnforce);
    llvm::Constant *Info = FinishViolationInfo(
        *this, getContext().BuildViolationObject(
                   Loc, "array subscript out of bounds", std::nullopt, FD));
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

// P3098: emit a postcondition's capture-construction DeclStmt, converting a
// construction exception into a post_capture violation. See the
// CodeGenFunction.h declaration for the contract.
void CodeGenFunction::EmitPostconditionCaptureInit(
    const ContractStmt *CS, ContractEvaluationSemantic Sem) {
  const DeclStmt *CapturesStmt = CS->getCapturesDeclStmt();
  if (!getLangOpts().Exceptions || !getLangOpts().ContractExceptions) {
    EmitStmt(CapturesStmt);
    return;
  }
  // Unlike the predicate-throw path (emitCheckForSemantic), which uses
  // StmtCanThrow to avoid try/catch overhead on a condition expression,
  // always wrap capture construction here: StmtCanThrow only special-cases
  // CallExpr and CXXBindTemporaryExpr, not a plain CXXConstructExpr, so it
  // false-negatives on the common "constructor body itself throws" case
  // (e.g. `Bad(int) { throw 42; }`) and would let the exception propagate
  // unguarded.

  Address FailedFlag = CreateTempAlloca(Builder.getInt1Ty(), CharUnits::One(),
                                        "contract.capture.failed");
  Builder.CreateStore(Builder.getFalse(), FailedFlag);
  ContractData->CaptureInitFailed.insert({CS, FailedFlag});

  llvm::Constant *ViolationInfo = BuildContractViolationInfo(*this, *CS);
  auto *Try =
      BuildContractCatchAllTry(CS->getCapturesDeclStmt()->getBeginLoc(), *this);

  // Catch body for a capture's construction try/catch: report the exception as
  // a post_capture violation.  Unlike the predicate-throw path there is no
  // predicate to re-check afterward -- construction either succeeded (no catch)
  // or failed, and 'observe' must skip the predicate entirely (recorded via
  // FailedFlag) rather than fall through to evaluate it.
  auto EmitCaptureCatchBody = [&] {
    if (Sem == Enforce || Sem == Observe || Sem == NoexceptEnforce ||
        Sem == NoexceptObserve) {
      EmitCxaContractViolationCall(
          ContractKind::Post, Sem, ExceptionRaised, ViolationInfo,
          /*IsNoExcept=*/Sem == NoexceptEnforce || Sem == NoexceptObserve,
          /*IsPostCapture=*/true);
      if (Sem == Observe || Sem == NoexceptObserve)
        Builder.CreateStore(Builder.getTrue(), FailedFlag);
    } else if (Sem == QuickEnforce) {
      CreateTrap(*this);
    } else {
      llvm_unreachable("Unhandled semantic");
    }
  };

  // Construct each capture in its own try/catch region, split into the
  // Alloca/Init/Cleanups steps ExitCXXTryStmt normally performs together via
  // EmitAutoVarDecl. This matters because the catch teardown requires the catch
  // scope it pushed to still be the top of the EHScopeStack -- but a
  // capture's destructor cleanup must persist past this function (Task 2's
  // ordering fix and the body-throws path both depend on it living until
  // the real function exit), so it cannot be pushed until after the catch
  // scope is torn down. Deferring EmitAutoVarCleanups until after the catch
  // teardown satisfies both constraints; a still-throwing later
  // capture in the same DeclStmt correctly unwinds this one via that
  // (by-then-active) cleanup, exactly like ordinary sequential construction.
  llvm::BasicBlock *DoneBB = nullptr;
  for (Decl *D : CapturesStmt->decls()) {
    auto *VD = cast<VarDecl>(D);
    AutoVarEmission Emission = EmitAutoVarAlloca(*VD);

    EnsureInsertPoint();
    EnterCXXTryStmt(*Try);
    EmitAutoVarInit(Emission);
    ExitCXXTryStmtWithCatchIR(*Try, EmitCaptureCatchBody);

    llvm::Value *Failed = Builder.CreateLoad(FailedFlag);
    llvm::BasicBlock *ContBB = createBasicBlock("contract.capture.cont");
    if (!DoneBB)
      DoneBB = createBasicBlock("contract.capture.done");
    Builder.CreateCondBr(Failed, DoneBB, ContBB);
    EmitBlock(ContBB);
    EmitAutoVarCleanups(Emission);
  }
  if (DoneBB) {
    Builder.CreateBr(DoneBB);
    EmitBlock(DoneBB);
  }
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

// Get (or create) the dynamic selector function and, when requested, emit a
// weak definition returning the compile-time default semantic.  Deduplicated
// once per unique symbol per TU.  The selector's ABI matches
// std::contracts::evaluation_semantic, whose underlying type is a 16-bit
// integer
// (__UINT16_TYPE__) in both libc++ and libstdc++, so it returns i16.  This
// width is fixed at 16 bits deliberately so that a "C"-linkage selector is ABI-
// compatible across toolchains (GCC likewise uses a 16-bit return type here).
// (The return type is not part of the mangled name.)
static llvm::Function *
getOrCreateDynamicSelector(CodeGenModule &CGM, StringRef Name, int Linkage,
                           bool ProvideWeak,
                           ContractEvaluationSemantic DefaultSem) {
  std::string Symbol;
  if (Linkage == 1)
    Symbol = Name.str(); // C linkage: verbatim symbol.
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
    B.CreateRet(llvm::ConstantInt::get(SemTy, static_cast<int>(DefaultSem)));
  }

  return Fn;
}

void CodeGenFunction::EmitContractStmtAsFullStmt(const ContractStmt &S) {
  assert(CurContract() == nullptr);

  ContractEvaluationSemantic Semantic = S.ensureRuntimeSemantic(
      getContext(), CurFuncDecl ? CurFuncDecl->getDeclContext() : nullptr);

  // P3595 dynamic selection: when the contract's runtime semantic is chosen by
  // a selector function at link/run time, emit the selector call plus a switch
  // that dispatches, per raw return value, to the precomputed effective
  // semantic's check body.  Task 3 already folded the label transform into the
  // per-value table (getDynTransform); codegen just consumes it.
  if (S.isDynamic()) {
    // Selector call.  'Semantic' (the eagerly-resolved compile-time default) is
    // the value the weak fallback returns.
    llvm::Function *Selector =
        getOrCreateDynamicSelector(CGM, S.getDynName(), S.getDynLinkage(),
                                   S.getDynProvideWeak(), Semantic);

    EnsureInsertPoint();
    llvm::Value *Raw = Builder.CreateCall(Selector->getFunctionType(), Selector,
                                          {}, "contract.dyn.raw");
    // Widen to the switch condition type (i32) for a uniform comparison.
    llvm::Value *RawExt = Builder.CreateZExt(Raw, IntTy, "contract.dyn.sem");

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
        Builder.CreateSwitch(RawExt, EnforceBB, /*NumCases=*/4);

    // Build one arm per raw value 1..4.  Deduplicate arm blocks by effective
    // semantic so each distinct check body is emitted at most once.
    llvm::DenseMap<int, llvm::BasicBlock *> SemToBlock;
    for (unsigned R = 1; R <= 4; ++R) {
      ContractEvaluationSemantic Eff = S.getDynTransform(R);
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

    // Emit the enforced-violation block: hand off to the shared enforce block
    // for this contract's kind (so the violation is reported as pre/post/assert
    // rather than a single hardcoded kind).
    EmitBlock(EnforceBB);
    ContractKind Kind = S.getContractKind();
    ContractData->GetSharedEnforceBlock(*this, Kind)
        .IncomingPHI->addIncoming(EnforceInfo, EnforceBB);
    Builder.CreateBr(GetSharedContractViolationEnforceBlock(Kind));

    EmitBlock(Continue);
    return;
  }

  emitCheckForSemantic(S, Semantic, /*ContinueBlock=*/nullptr);
}

/// Build the per-assertion data block the runtime walks, as a stack copy of
/// the global block with the label's facet pointers appended.  Returns the
/// pointer to hand the entry point: the extended block when the label carries
/// a local handler or a query, and the plain global block otherwise.
///
/// Both detection paths need this.  The predicate-false path always had it;
/// the exception path did not, and passed the bare global block instead, so a
/// throwing predicate reported through a block carrying neither facet -- the
/// local violation handler was never dispatched to, and query_control_object
/// answered null.  GCC has always passed one shared block to both entry
/// points.
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
  const auto *LabelRD = S.getLabelExpr()->getType()->getAsCXXRecordDecl();
  llvm::Function *HandlerTrampoline =
      (HasLocalHandler && LabelRD)
          ? getOrCreateLocalHandlerTrampoline(CGM, LabelRD)
          : nullptr;
  llvm::Function *QueryTrampoline =
      (HasQuery && LabelRD) ? getOrCreateQueryTrampoline(CGM, LabelRD)
                            : nullptr;

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
// observe semantics -- quick_enforce calls no handler, and the D4298 noexcept_
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
  RO_Fail,        ///< Could not be analysed, or something observable happened.
};

/// How deep the walk will follow calls before giving up.  A handler that
/// delegates more than this far is not a shape worth proving, and the limit
/// doubles as the termination guard for mutual recursion.
static const int RethrowMaxDepth = 8;

/// std::contracts::assertion_kind for KIND.  The ABI encodes the kind in the
/// entry point *name* rather than a number, so unlike the other three seeded
/// properties this one needs an explicit mapping; it follows
/// GetCxaEntryPointName's own post-capture handling so the two cannot drift.
static int64_t assertionKindValue(ContractKind Kind, bool IsPostCapture) {
  if (IsPostCapture)
    return 6; // post_capture
  switch (Kind) {
  case ContractKind::Pre:
    return 1;
  case ContractKind::Post:
    return 2;
  case ContractKind::Assert:
    return 3;
  case ContractKind::Implicit:
    return 7;
  }
  return 0; // unspecified
}

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

  if (const auto *UO = dyn_cast<UnaryOperator>(E)) {
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

  if (const auto *AS = dyn_cast<AttributedStmt>(S))
    return walkStmt(AS->getSubStmt());

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
      const auto *LHS = dyn_cast<DeclRefExpr>(BO->getLHS()->IgnoreParenImpCasts());
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
  if (CGF.getLangOpts().ContractDisableRethrowShortcut)
    return false;

  // Only the two semantics whose handler may legitimately let an exception
  // escape.
  if (Sem != Enforce && Sem != Observe)
    return false;

  if (!S.hasLocalHandler() || !S.getLabelExpr())
    return false;

  const CXXRecordDecl *LabelRD = S.getLabelExpr()->getType()->getAsCXXRecordDecl();
  if (!LabelRD)
    return false;

  const CXXMethodDecl *HCV = findLocalViolationHandler(LabelRD);
  if (!HCV || HCV->isVirtual())
    return false;

  // A noexcept handler cannot rethrow -- it would terminate.
  if (HCV->getType()->castAs<FunctionProtoType>()->isNothrow())
    return false;

  const Stmt *Body = HCV->getBody();
  if (!Body || HCV->getNumParams() != 1)
    return false;

  RethrowAnalysis Analysis(HCV->getParamDecl(0), Sem,
                           assertionKindValue(S.getContractKind(),
                                              IsPostCapture));
  return Analysis.walkStmt(Body) == RO_Rethrown;
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
        return Shared;
    }
      LLVM_FALLTHROUGH;
    case Observe:
    case NoexceptEnforce:
    case NoexceptObserve:
      return Inline;
    case QuickEnforce:
      return Shared;
    case ContractEvaluationSemantic::Ignore:
    case ContractEvaluationSemantic::Assume:
      llvm_unreachable("unhandled semantic");
    }
    llvm_unreachable("unhandled contract evaluation semantic");
  }();

  auto Violation = [&]() {
    switch (Style) {
    case Inline:
      return createBasicBlock("contract.violation");
    case Shared:
      assert(Semantic == Enforce || Semantic == QuickEnforce);
      return Semantic == Enforce
                 ? GetSharedContractViolationEnforceBlock(S.getContractKind())
                 : GetSharedContractViolationTrapBlock();
    }
    llvm_unreachable("unhandled contract emission style");
  }();

  llvm::BasicBlock *End = createBasicBlock("contract.end");

  llvm::Constant *ViolationInfo = nullptr;
  if (Semantic != ContractEvaluationSemantic::QuickEnforce) {
    auto *ViolationGV = getContext().BuildViolationObject(
        &S, dyn_cast_or_null<FunctionDecl>(CurFuncDecl));
    llvm::Constant *DataGV =
        CGM.GetAddrOfUnnamedGlobalConstantDecl(ViolationGV, "contract.loc")
            .getPointer();

    // Always use the basic (3-entry) descriptor for the global constant.
    // If a local handler is present, the extended descriptor will be used
    // in the stack-allocated data block instead.
    llvm::Constant *DescTable =
        getOrCreateDescriptorTable(CGM,
                                   /*HasLocalHandler=*/false);
    // We need to create a new global with the descriptor pointer filled in.
    // The ViolationGV from AST has a null descriptor; we create a copy
    // with the descriptor pointer patched.
    if (auto *OrigGV = dyn_cast<llvm::GlobalVariable>(DataGV)) {
      // Get the original initializer.
      llvm::Constant *OrigInit = OrigGV->getInitializer();
      if (auto *OrigStruct = dyn_cast<llvm::ConstantStruct>(OrigInit)) {
        SmallVector<llvm::Constant *, 8> NewFields;
        for (unsigned i = 0; i < OrigStruct->getNumOperands(); ++i) {
          if (i == 0) {
            // Replace null descriptor with actual descriptor pointer.
            NewFields.push_back(DescTable);
          } else {
            NewFields.push_back(OrigStruct->getOperand(i));
          }
        }
        llvm::Constant *NewInit =
            llvm::ConstantStruct::get(OrigStruct->getType(), NewFields);

        // Create a new global with the patched initializer.
        auto *PatchedGV = new llvm::GlobalVariable(
            CGM.getModule(), OrigStruct->getType(),
            /*isConstant=*/true, llvm::GlobalValue::InternalLinkage, NewInit,
            OrigGV->getName() + ".patched");
        PatchedGV->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);

        ViolationInfo = PatchedGV;
      } else {
        ViolationInfo = DataGV;
      }
    } else {
      ViolationInfo = DataGV;
    }
  }

  CurrentContractInfo CCInfo{};
  CCInfo.Contract = &S;
  CCInfo.Style = Style;
  CCInfo.Semantic = Semantic;
  CCInfo.Violation = Violation;
  CCInfo.End = End;
  CCInfo.ViolationInfoGV = ViolationInfo;
  CurrentContractRAII CurContractRAII(*this, CCInfo);

  bool IsPostCapture =
      (S.getContractKind() == ContractKind::Post && S.hasCaptures());

  llvm::Value *BranchOn;
  // If the label's local violation handler answers an evaluation_exception by
  // rethrowing, catching the predicate's exception only to hand it to that
  // handler is pure overhead -- let it propagate instead.
  if (getLangOpts().Exceptions && getLangOpts().ContractExceptions &&
      StmtCanThrow(S.getCond()) &&
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

  if (Style == Shared && Semantic == Enforce) {
    // assert(!getLangOpts().Exceptions);
    EnsureInsertPoint();
    ContractData->GetSharedEnforceBlock(*this, S.getContractKind())
        .IncomingPHI->addIncoming(ViolationInfo, Builder.GetInsertBlock());
  }

  Builder.CreateCondBr(BranchOn, End, Violation);

  // If we're creating a trap, the violation block will be created once for the
  // function. Otherwise, we need to create a call to the violation handler.
  if (Style == Inline) {
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

// ===----------------------------------------------------------------------===//
// P3097: Virtual Function Contract Wrappers
// ===----------------------------------------------------------------------===//

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

  // Phase 1: Emit interface preconditions and initialize postcondition
  // captures in lexical order (same as normal function prologue).
  if (MD->hasContracts()) {
    for (auto *CS : MD->getContracts()->contracts()) {
      if (CS->getContractKind() == ContractKind::Pre) {
        EmitStmt(CS);
      } else if (CS->getContractKind() == ContractKind::Post &&
                 CS->hasCaptures()) {
        ContractEvaluationSemantic Sem = CS->ensureRuntimeSemantic(
            getContext(), MD ? MD->getDeclContext() : nullptr);
        // 'assume' lowers to 'ignore' (no check emitted); the capture must
        // not be constructed either -- see the matching gate in
        // CodeGenFunction::GenerateCode.
        if (Sem != ContractEvaluationSemantic::Ignore &&
            Sem != ContractEvaluationSemantic::Assume)
          EmitPostconditionCaptureInit(CS, Sem);
      }
    }
  }

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
  EmitPostContracts(RetVal);

  // Emit return via the thunk pattern.
  if (!ResultType->isVoidType() && Slot.isNull())
    CGM.getCXXABI().EmitReturnFromThunk(*this, RV, ResultType);

  // Disable tail call / autorelease.
  AutoreleaseResult = false;

  // Clear thunk state and finish.
  CurCodeDecl = nullptr;
  CurFuncDecl = nullptr;
  FinishFunction();
}
