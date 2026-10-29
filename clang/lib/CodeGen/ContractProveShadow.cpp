//===--- ContractProveShadow.cpp - P4405 proof shadows --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The proof shadow strategy, -fcontracts-prove-shadow: the code generation
// side (see ContractProveShadow.h for the design; ContractShadowDecide.cpp is
// the back end side).
//
// When the module is optimized (-O1 and up, and none of the exclusions in
// isContractShadowActive), an evaluation with prove
// or diagnose that folding does not settle is deferred: it becomes an
// obligation with an id, recorded in the module's obligation list, and the
// function emitting it is queued for a proof shadow, unless the function is
// one no shadow is made of (isContractShadowable).  A caller-side
// precondition is deferred only when the callee's parameters it names, and
// `this`, can be given the call's arguments again (no side effects; scalar
// or reference parameters; not a lambda; a constructor or destructor call's
// object for `this`).
//
// The real function's code is ignore's, except under
// -fcontracts-prove-assume, where the evaluation leaves
// `if (__clang_contract_proven(id)) llvm.assume(pred)`, which the back end
// keeps for a sound proof or deletes, before the module is optimized.
//
// From Release, each queued function is emitted again into a new internal
// function, "<name>.contract.proof", carrying the attribute
// "clang.contract.proof": the same GlobalDecl through GenerateCode, with
// diagnostics suppressed and no debug information.  There an obligation is
// `if (!pred) __clang_contract_unproven(id)` (noreturn, so a path where the
// predicate is false ends there and cannot make later code unreachable), and
// an earlier evaluation is an assumption, `if (!pred) unreachable`: assume
// (guaranteed), ignore, observe and noexcept_observe (not guaranteed, which
// makes every proof in the shadow conditional); ignore and assume only for a
// predicate without side effects, since they do not evaluate it.  enforce,
// noexcept_enforce and quick_enforce need nothing: their violation paths do
// not return.  The calls and stores emitted for an obligation's predicate
// carry "clang.contract.pred" metadata, so that the back end can tell
// whether a proof relies on a predicate's side effects.
//
// While shadows are made, a real function emitting an enforce,
// noexcept_enforce or quick_enforce check is listed in the module's premise
// list ("clang.contract.premises"), unless the assertion's allowed set holds
// no continuing semantic (ignore, observe, noexcept_observe, diagnose,
// prove): another TU's copy of such a function may not check, so the back
// end relies on this copy's check only where the real code runs this copy.
//
// D4405's override shadows (ContractProveOverride.cpp) are emitted here too,
// as "<name>.contract.override.<pair>", with the attribute
// "clang.contract.override": there an ordinary obligation is a premise,
// `if (!pred) unreachable', listed in the shadow's "clang.contract.relied"
// metadata so that the back end makes its proofs conditional unless every
// one is proven soundly, and an override pair's assertions of B::f see
// D::f's parameters and the B subobject (OverrideBinding).  Without override
// shadows, an override pair's B::f postconditions are this strategy's
// obligations of D::f, never assumptions in the real code.
//
// While shadows are made, every evaluation decided at once (folding's
// "always false", an evaluation that cannot be deferred) is recorded too
// instead of diagnosed, so that the back end diagnoses everything in the
// order code generation met it.  An error that stops compilation before the
// back end runs makes reportContractProofsNow diagnose them instead.
//
//===----------------------------------------------------------------------===//

#include "ContractProveShadow.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/CodeGenOptions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include <optional>

using namespace clang;
using namespace CodeGen;
namespace cs = contract_shadow;

//===----------------------------------------------------------------------===//
// When shadows are made
//===----------------------------------------------------------------------===//

/// Whether proof shadows, of any kind, can be made in this module: the
/// module is optimized (-O1 and up; with the LLVM passes disabled too, as
/// -save-temps does for the step that writes the bitcode, the proof module
/// being optimized at the -O level all the same), and nothing would
/// duplicate state a second emission
/// of a function cannot share (front-end profile instrumentation or use,
/// coverage mapping), stream the shadows' optimization remarks to a file, or
/// lower functions through another runtime (OpenMP, CUDA/HIP, OpenCL, SYCL,
/// HLSL, the incremental compiler).
bool CodeGen::canMakeContractShadows(CodeGenModule &CGM) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  if (St.CanShadow >= 0)
    return St.CanShadow;
  const LangOptions &LO = CGM.getLangOpts();
  const CodeGenOptions &CO = CGM.getCodeGenOpts();
  bool Can = CO.OptimizationLevel > 0 && !LO.OpenMP && !LO.CUDA && !LO.OpenCL &&
             !LO.SYCLIsDevice && !LO.HLSL && !LO.IncrementalExtensions &&
             !CO.hasProfileClangInstr() && !CO.hasProfileClangUse() &&
             !CO.CoverageMapping && CO.OptRecordFile.empty();
  St.CanShadow = Can;
  return Can;
}

/// Whether proof shadows are made in this module at all: the flag, and
/// canMakeContractShadows.
static bool isContractShadowActive(CodeGenModule &CGM) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  if (St.Active >= 0)
    return St.Active;
  bool Active =
      CGM.getLangOpts().ContractsProveShadow && canMakeContractShadows(CGM);
  St.Active = Active;
  return Active;
}

bool CodeGenFunction::isContractShadowable() const {
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CurGD.getDecl());
  if (!FD || !CurFn || CurFn->getName() != CGM.getMangledName(CurGD))
    return false;
  if (FD->isMultiVersion() || FD->isInlineBuiltinDeclaration() ||
      FD->hasAttr<OptimizeNoneAttr>() || FD->hasAttr<NakedAttr>())
    return false;
  if (const Stmt *Body = FD->getBody(); Body && isa<CoroutineBodyStmt>(Body))
    return false;
  return true;
}

//===----------------------------------------------------------------------===//
// Caller-side substitution
//===----------------------------------------------------------------------===//

namespace {
/// What a caller-side precondition's predicate needs from the call.
struct CallerSideUses {
  /// The callee's parameters it names, as indexes.
  llvm::SmallVector<unsigned, 4> Params;
  /// Whether it uses `this`.
  bool UsesThis = false;
};
} // namespace

/// Collect what \p Pred, a precondition of \p Callee, uses; false if it names
/// the parameters of more than one of \p Callee's declarations.
static bool collectCallerSideUses(const Expr *Pred, const FunctionDecl *Callee,
                                  CallerSideUses &Uses) {
  const FunctionDecl *Owner = nullptr;
  const Decl *Canon = Callee->getCanonicalDecl();
  llvm::SmallVector<const Stmt *, 16> Work{Pred};
  while (!Work.empty()) {
    const Stmt *S = Work.pop_back_val();
    if (!S)
      continue;
    if (isa<CXXThisExpr>(S))
      Uses.UsesThis = true;
    if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
      if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
        if (const auto *FD = dyn_cast<FunctionDecl>(PVD->getDeclContext()))
          if (FD->getCanonicalDecl() == Canon) {
            if (Owner && Owner != FD)
              return false;
            Owner = FD;
            unsigned I = PVD->getFunctionScopeIndex();
            if (!llvm::is_contained(Uses.Params, I))
              Uses.Params.push_back(I);
          }
    for (const Stmt *Child : S->children())
      Work.push_back(Child);
  }
  return true;
}

/// The arguments of \p CallSite aligned with the callee's parameters, and its
/// implicit object argument, if any.  D4405: a constructor call's arguments
/// (its object is CodeGenFunction::ContractCallObject); an implicit
/// destructor call has none.
static void getCallArguments(const Expr *CallSite,
                             SmallVectorImpl<const Expr *> &Args,
                             const Expr *&Object) {
  Object = nullptr;
  if (const auto *CCE = dyn_cast<CXXConstructExpr>(CallSite)) {
    Args.append(CCE->arg_begin(), CCE->arg_end());
    return;
  }
  const auto *CE = dyn_cast<CallExpr>(CallSite);
  if (!CE)
    return;
  if (const auto *MCE = dyn_cast<CXXMemberCallExpr>(CE)) {
    Object = MCE->getImplicitObjectArgument();
    Args.append(CE->arg_begin(), CE->arg_end());
    return;
  }
  if (const auto *OCE = dyn_cast<CXXOperatorCallExpr>(CE))
    if (const auto *MD =
            dyn_cast_or_null<CXXMethodDecl>(OCE->getDirectCallee()))
      if ((MD->isImplicitObjectMemberFunction() || MD->isStatic()) &&
          OCE->getNumArgs() > 0) {
        // A static operator's object argument is evaluated and ignored.
        if (!MD->isStatic())
          Object = OCE->getArg(0);
        Args.append(CE->arg_begin() + 1, CE->arg_end());
        return;
      }
  Args.append(CE->arg_begin(), CE->arg_end());
}

/// Whether \p Pred, a predicate of a lambda's call operator, names one of the
/// lambda's captures: a variable of an enclosing scope, or `this'.
static bool namesLambdaCapture(const Expr *Pred) {
  llvm::SmallVector<const Stmt *, 16> Work{Pred};
  while (!Work.empty()) {
    const Stmt *S = Work.pop_back_val();
    if (!S)
      continue;
    if (isa<CXXThisExpr>(S))
      return true;
    if (const auto *DRE = dyn_cast<DeclRefExpr>(S);
        DRE && DRE->refersToEnclosingVariableOrCapture())
      return true;
    for (const Stmt *Child : S->children())
      Work.push_back(Child);
  }
  return false;
}

/// Whether the caller-side precondition \p Q can be emitted at its call with
/// the call's arguments for the parameters: each argument it needs is
/// evaluated again, so must have no side effects, and only a scalar
/// parameter (a copy of the argument) or a reference (to the argument) is
/// given one.  A constructor or destructor call (D4405: whatever its
/// arguments, none included) gives `this' its object, when there is one
/// (CodeGenFunction::ContractCallObject).  For a lambda's call operator only
/// when the predicate names no capture (captures are not modelled).
static bool canSubstituteCallerSide(const ContractProofQuery &Q) {
  const Expr *CE = Q.CallSite;
  const FunctionDecl *Callee = Q.Callee;
  if (!CE || !Callee)
    return false;
  // A constructor is called by a CXXConstructExpr; a destructor by a member
  // call or implicitly (an expression standing for the call); anything else
  // by a call.
  if (isa<CXXConstructorDecl>(Callee) ? !isa<CXXConstructExpr>(CE)
                                      : !isa<CallExpr>(CE) &&
                                            !isa<CXXDestructorDecl>(Callee))
    return false;
  if (const auto *MD = dyn_cast<CXXMethodDecl>(Callee))
    if ((isLambdaCallOperator(MD) && namesLambdaCapture(Q.Predicate)) ||
        (MD->isExplicitObjectMemberFunction() && isa<CXXMemberCallExpr>(CE)))
      return false;
  CallerSideUses Uses;
  if (!collectCallerSideUses(Q.Predicate, Callee, Uses))
    return false;
  SmallVector<const Expr *, 8> Args;
  const Expr *Object;
  getCallArguments(CE, Args, Object);
  for (unsigned I : Uses.Params) {
    if (I >= Args.size() || I >= Callee->getNumParams())
      return false;
    const Expr *Arg = Args[I];
    QualType T = Callee->getParamDecl(I)->getType();
    if (Arg->HasSideEffects(Q.Ctx) || T.isVolatileQualified())
      return false;
    if (T->isReferenceType()) {
      if (!Arg->isGLValue())
        return false;
    } else if (!CodeGenFunction::hasScalarEvaluationKind(T)) {
      return false;
    }
  }
  if (Uses.UsesThis && !isa<CallExpr>(CE))
    return Q.CGF && Q.CGF->ContractCallObject;
  if (Uses.UsesThis) {
    const auto *MD = dyn_cast<CXXMethodDecl>(Callee);
    if (!Object || !MD || !MD->isImplicitObjectMemberFunction() ||
        Object->HasSideEffects(Q.Ctx))
      return false;
  }
  return true;
}

namespace {
/// Gives a caller-side precondition's predicate, emitted at the call, the
/// call's arguments for the callee's parameters and its object for `this`,
/// for the scope of the object.
class CallerSideBinding {
  CodeGenFunction &CGF;
  llvm::DenseMap<const Decl *, Address> &Map;
  llvm::SmallVector<std::pair<const ParmVarDecl *, std::optional<Address>>, 4>
      Saved;
  llvm::Value *&ThisValue;
  CharUnits &ThisAlignment;
  llvm::Value *SavedThis;
  CharUnits SavedAlignment;

public:
  CallerSideBinding(CodeGenFunction &CGF,
                    llvm::DenseMap<const Decl *, Address> &Map,
                    llvm::Value *&ThisValue, CharUnits &ThisAlignment)
      : CGF(CGF), Map(Map), ThisValue(ThisValue), ThisAlignment(ThisAlignment),
        SavedThis(ThisValue), SavedAlignment(ThisAlignment) {}

  void bind(const Expr *Pred, const Expr *CE, const FunctionDecl *Callee) {
    CallerSideUses Uses;
    collectCallerSideUses(Pred, Callee, Uses);
    SmallVector<const Expr *, 8> Args;
    const Expr *Object;
    getCallArguments(CE, Args, Object);
    // The predicate names the parameters of the declaration it was written
    // on; find them by index on that declaration.
    llvm::SmallVector<const ParmVarDecl *, 4> Parms;
    llvm::SmallVector<const Stmt *, 16> Work{Pred};
    const Decl *Canon = Callee->getCanonicalDecl();
    while (!Work.empty()) {
      const Stmt *S = Work.pop_back_val();
      if (!S)
        continue;
      if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
        if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
          if (const auto *FD = dyn_cast<FunctionDecl>(PVD->getDeclContext()))
            if (FD->getCanonicalDecl() == Canon &&
                !llvm::is_contained(Parms, PVD))
              Parms.push_back(PVD);
      for (const Stmt *Child : S->children())
        Work.push_back(Child);
    }
    for (const ParmVarDecl *PVD : Parms) {
      const Expr *Arg = Args[PVD->getFunctionScopeIndex()];
      QualType T = PVD->getType();
      Address A = Address::invalid();
      if (T->isReferenceType()) {
        // A reference parameter's slot holds the address of its referent.
        llvm::Value *Ptr = CGF.EmitLValue(Arg).getPointer(CGF);
        A = CGF.CreateTempAlloca(Ptr->getType(), CGF.getPointerAlign(),
                                 "contract.proof.ref");
        CGF.Builder.CreateStore(Ptr, A);
      } else {
        A = CGF.CreateMemTemp(T, "contract.proof.arg");
        CGF.EmitStoreOfScalar(CGF.EmitScalarExpr(Arg), CGF.MakeAddrLValue(A, T),
                              /*isInit=*/true);
      }
      auto It = Map.find(PVD);
      if (It != Map.end()) {
        Saved.push_back({PVD, It->second});
        It->second = A;
      } else {
        Saved.push_back({PVD, std::nullopt});
        Map.insert({PVD, A});
      }
    }
    if (Uses.UsesThis) {
      const auto *MD = cast<CXXMethodDecl>(Callee);
      if (!Object)
        ThisValue = CGF.ContractCallObject;
      else
        ThisValue = Object->getType()->isPointerType() && Object->isPRValue()
                        ? CGF.EmitScalarExpr(Object)
                        : CGF.EmitLValue(Object).getPointer(CGF);
      ThisAlignment = CGF.CGM.getClassPointerAlignment(MD->getParent());
    }
  }

  ~CallerSideBinding() {
    for (auto &[PVD, Old] : llvm::reverse(Saved)) {
      if (Old)
        Map.find(PVD)->second = *Old;
      else
        Map.erase(PVD);
    }
    ThisValue = SavedThis;
    ThisAlignment = SavedAlignment;
  }
};

/// D4405: gives an assertion of an override pair's B::f, emitted in a shadow
/// of D::f, D::f's parameters (or their snapshots, ContractOverrideSnapshot)
/// for B::f's, by position, and the B subobject for `this', for the scope of
/// the object.  (B::f's result is the caller's: CodeGenFunction::ReturnValue.)
class OverrideBinding {
  CodeGenFunction &CGF;
  llvm::DenseMap<const Decl *, Address> &Map;
  llvm::SmallVector<std::pair<const ParmVarDecl *, std::optional<Address>>, 4>
      Saved;
  llvm::Value *&ThisValue;
  CharUnits &ThisAlignment;
  llvm::Value *SavedThis;
  CharUnits SavedAlignment;

public:
  OverrideBinding(CodeGenFunction &CGF,
                  llvm::DenseMap<const Decl *, Address> &Map,
                  llvm::Value *&ThisValue, CharUnits &ThisAlignment)
      : CGF(CGF), Map(Map), ThisValue(ThisValue), ThisAlignment(ThisAlignment),
        SavedThis(ThisValue), SavedAlignment(ThisAlignment) {}

  void bind(const Expr *Pred, const ContractOverridePair &P,
            const ContractOverrideSnapshot *Snap) {
    const Decl *Canon = P.Base->getCanonicalDecl();
    llvm::SmallVector<const ParmVarDecl *, 4> Parms;
    llvm::SmallVector<const Stmt *, 16> Work{Pred};
    while (!Work.empty()) {
      const Stmt *S = Work.pop_back_val();
      if (!S)
        continue;
      if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
        if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
          if (const auto *FD = dyn_cast<FunctionDecl>(PVD->getDeclContext()))
            if (FD->getCanonicalDecl() == Canon &&
                !llvm::is_contained(Parms, PVD))
              Parms.push_back(PVD);
      for (const Stmt *Child : S->children())
        Work.push_back(Child);
    }
    for (const ParmVarDecl *PVD : Parms) {
      unsigned I = PVD->getFunctionScopeIndex();
      if (I >= P.Derived->getNumParams())
        continue;
      Address A = Snap && I < Snap->Params.size() && Snap->Params[I].isValid()
                      ? Snap->Params[I]
                      : CGF.GetAddrOfLocalVar(P.Derived->getParamDecl(I));
      auto It = Map.find(PVD);
      if (It != Map.end()) {
        Saved.push_back({PVD, It->second});
        It->second = A;
      } else {
        Saved.push_back({PVD, std::nullopt});
        Map.insert({PVD, A});
      }
    }
    ThisValue =
        Snap && Snap->This ? Snap->This : getContractOverrideThis(CGF, P);
    ThisAlignment = CGF.CGM.getClassPointerAlignment(P.Base->getParent());
  }

  ~OverrideBinding() {
    for (auto &[PVD, Old] : llvm::reverse(Saved)) {
      if (Old)
        Map.find(PVD)->second = *Old;
      else
        Map.erase(PVD);
    }
    ThisValue = SavedThis;
    ThisAlignment = SavedAlignment;
  }
};
} // namespace

llvm::Value *CodeGen::getContractOverrideThis(CodeGenFunction &CGF,
                                              const ContractOverridePair &P) {
  return CGF
      .GetAddressOfBaseClass(CGF.LoadCXXThisAddress(), P.Derived->getParent(),
                             P.Path.begin(), P.Path.end(),
                             /*NullCheckValue=*/false, P.Derived->getLocation())
      .emitRawPointer(CGF);
}

/// D4405: whether \p S is an assertion of \p P's B::f.
static bool isOverrideBaseContract(const ContractStmt &S,
                                   const ContractOverridePair &P) {
  return P.BaseWithContracts &&
         llvm::is_contained(P.BaseWithContracts->getContracts()->contracts(),
                            &S);
}

//===----------------------------------------------------------------------===//
// Obligations
//===----------------------------------------------------------------------===//

ContractShadowModuleState &CodeGenModule::getContractShadowState() {
  if (!ContractShadowData)
    ContractShadowData = std::make_shared<ContractShadowModuleState>();
  return *ContractShadowData;
}

/// The locations an obligation is diagnosed with.
static void getObligationLocations(const ContractObligation &O,
                                   SourceLocation &Loc, SourceRange &Range,
                                   SourceRange &PredRange) {
  PredRange = O.Contract->getCond()->getSourceRange();
  if (O.CallSite) {
    Loc = O.CallSite->getExprLoc();
    Range = O.CallSite->getSourceRange();
  } else {
    Loc = O.Contract->getKeywordLoc();
    Range = PredRange;
  }
}

/// Write obligation \p Id's record to the module's list, whose operand \p Id
/// it is.
static void writeObligation(CodeGenModule &CGM, unsigned Id) {
  const ContractObligation &O = CGM.getContractShadowState().Obligations[Id];
  SourceLocation Loc;
  SourceRange Range, PredRange;
  getObligationLocations(O, Loc, Range, PredRange);
  llvm::LLVMContext &Ctx = CGM.getLLVMContext();
  auto Field = [&](uint64_t V) -> llvm::Metadata * {
    return llvm::ConstantAsMetadata::get(
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(Ctx), V));
  };
  llvm::Metadata *Fields[] = {
      Field(Loc.getRawEncoding()),
      Field(Range.getBegin().getRawEncoding()),
      Field(Range.getEnd().getRawEncoding()),
      Field(PredRange.getBegin().getRawEncoding()),
      Field(PredRange.getEnd().getRawEncoding()),
      Field(O.Flags),
      Field(O.Decision),
      Field(O.DiagContext),
      llvm::MDString::get(Ctx, O.Override ? O.Override->BaseName : "")};
  llvm::MDNode *N = llvm::MDTuple::get(Ctx, Fields);
  llvm::NamedMDNode *List =
      CGM.getModule().getOrInsertNamedMetadata(cs::ObligationsMD);
  if (Id < List->getNumOperands())
    List->setOperand(Id, N);
  else
    List->addOperand(N);
}

unsigned CodeGen::addContractObligation(CodeGenModule &CGM,
                                        const ContractProofQuery &Q,
                                        unsigned Decision) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  unsigned Id = St.Obligations.size();
  ContractObligation O{&Q.Contract, Q.CallSite, Q.Callee, Q.Semantic};
  if (Q.Semantic == ContractEvaluationSemantic::Prove)
    O.Flags |= cs::Prove;
  if (Q.CallSite)
    O.Flags |= cs::CallerSide;
  if (Q.CGF && Q.CGF->isInContractShadowLoop())
    O.Flags |= cs::InLoop;
  if (Q.Override) {
    O.Flags |= cs::Override;
    if (Q.OverrideShadow)
      O.Flags |= cs::OverrideShadow;
    O.Override = Q.Override;
  }
  O.Decision = Decision;
  O.DiagContext = CGM.getContext().getContractDiagnosticContext(
      Q.CGF ? Q.CGF->CurCodeDecl : nullptr);
  St.Obligations.push_back(O);
  St.ObligationIds.insert({{&Q.Contract, Q.CallSite}, Id});
  writeObligation(CGM, Id);
  return Id;
}

//===----------------------------------------------------------------------===//
// The strategy
//===----------------------------------------------------------------------===//

static bool isShadowEnabled(const LangOptions &LangOpts) {
  return LangOpts.ContractsProveShadow;
}

/// Defer \p Q to the shadow if it can be: shadows are made in this module,
/// this is a real function that can have one (not a shadow, and not inside
/// a predicate being emitted), and, for a caller-side evaluation, it is a
/// precondition whose arguments can be substituted.  A postcondition with
/// captures is not (its captures are not initialized under prove or
/// diagnose).  Of an override pair's obligations (D4405), only those of
/// D::f itself are this strategy's, without override shadows: a
/// postcondition of B::f, proven in D::f's shadow with nothing given; D::f's
/// preconditions must then fold (DECISIONS.md E157).
static ContractProofResult proveByShadow(const ContractProofQuery &Q) {
  CodeGenFunction *CGF = Q.CGF;
  if (!CGF || CGF->ContractShadow || !isContractShadowActive(CGF->CGM) ||
      !CGF->isContractShadowable() || Q.Predicate->isValueDependent() ||
      Q.Contract.hasCaptures())
    return ContractProofResult::Unproven;
  if (Q.Override)
    return !Q.OverrideShadow &&
                   Q.Contract.getContractKind() == ContractKind::Post
               ? ContractProofResult::Deferred
               : ContractProofResult::Unproven;
  if (Q.CallSite && (Q.Contract.getContractKind() != ContractKind::Pre ||
                     !canSubstituteCallerSide(Q)))
    return ContractProofResult::Unproven;
  return ContractProofResult::Deferred;
}

static bool placeShadowObligation(const ContractProofQuery &Q, bool Defer) {
  CodeGenFunction *CGF = Q.CGF;
  // An override shadow's obligation is the override strategy's.
  if (!CGF || (Q.Override && Q.OverrideShadow))
    return false;
  CodeGenModule &CGM = CGF->CGM;
  ContractShadowModuleState &St = CGM.getContractShadowState();
  unsigned Id;
  auto It = St.ObligationIds.find({&Q.Contract, Q.CallSite});
  if (It == St.ObligationIds.end()) {
    if (!Defer)
      return false;
    Id = addContractObligation(CGM, Q, cs::Deferred);
  } else {
    Id = It->second;
    if (St.Obligations[Id].Decision != cs::Deferred)
      return false;
  }

  bool InShadow = CGF->ContractShadow && CGF->ContractShadow->InShadow;
  if (!InShadow) {
    // A real function: it needs a shadow, which this one may not be able to
    // have (a second emission of the evaluation somewhere else).
    if (!CGF->isContractShadowable()) {
      St.Obligations[Id].Flags |= cs::Unshadowed;
      writeObligation(CGM, Id);
      return true;
    }
    StringRef Name = CGF->CurFn->getName();
    if (St.PendingNames.insert(Name).second)
      St.Pending.emplace_back(CGF->CurGD, Name.str(), 0u);
    // Under -fcontracts-prove-assume, the assumption the back end may keep;
    // never for a predicate with side effects of its own (one could not be
    // proven soundly), nor when this module is not optimized, nor for an
    // override pair's obligation (D4405: D::f serves calls that are not
    // through B::f too, and B::f's postconditions are checked elsewhere).
    if (Q.Override || !CGM.getLangOpts().ContractsProveAssume ||
        CGM.getCodeGenOpts().DisableLLVMPasses ||
        Q.Predicate->HasSideEffects(Q.Ctx, /*IncludePossibleEffects=*/false))
      return true;
  }
  // D4405: an override shadow proves its pair's obligations only; an
  // ordinary obligation there is a premise, on whose result its proofs
  // depend (ContractShadowDecide.cpp), and it is never decided there.
  if (InShadow && CGF->ContractShadow->Override) {
    if (!Q.Override &&
        !Q.Predicate->HasSideEffects(Q.Ctx, /*IncludePossibleEffects=*/false))
      CGF->emitContractShadowPremise(Id, Q.Contract, Q.CallSite, Q.Callee);
    return true;
  }
  CGF->emitContractShadowPredicate(Id, Q.Contract, Q.CallSite, Q.Callee,
                                   Q.Override);
  return true;
}

/// While shadows are made, take every diagnostic decided at once, so that the
/// back end issues it in order with the deferred ones.  One met while a
/// shadow or a predicate is emitted is dropped: an evaluation the real code
/// emits has been decided already, and one that only a predicate holds (a
/// caller-side evaluation of a call in the predicate) is not evaluated by
/// the program.
static bool takeShadowDiagnostic(const ContractProofQuery &Q,
                                 ContractProofResult R) {
  if (!Q.CGF || !isContractShadowActive(Q.CGF->CGM))
    return false;
  if (Q.CGF->ContractShadow)
    return true;
  addContractObligation(Q.CGF->CGM, Q,
                        R == ContractProofResult::AlwaysFalse
                            ? cs::TakenAlwaysFalse
                            : cs::TakenUnproven);
  return true;
}

const ContractProofStrategy CodeGen::ContractProveShadowStrategy = {
    "shadow",      /*MayDependOnAssumptions=*/true, isShadowEnabled,
    proveByShadow, placeShadowObligation,           takeShadowDiagnostic};

//===----------------------------------------------------------------------===//
// Emission
//===----------------------------------------------------------------------===//

void CodeGenFunction::noteContractShadowInstruction(
    llvm::Instruction *I) const {
  const ContractShadowFunction &S = *ContractShadow;
  if (S.PredicateTag < 0 || (!isa<llvm::CallBase>(I) && !I->mayWriteToMemory()))
    return;
  llvm::LLVMContext &Ctx = I->getContext();
  I->setMetadata(cs::PredicateMD,
                 llvm::MDNode::get(
                     Ctx, llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
                              llvm::Type::getInt32Ty(Ctx), S.PredicateTag))));
}

/// The declaration of \p Name, \p Ty, nounwind (and noreturn if \p NoReturn).
static llvm::Function *getMarker(CodeGenModule &CGM, StringRef Name,
                                 llvm::FunctionType *Ty, bool NoReturn) {
  llvm::Module &M = CGM.getModule();
  if (llvm::Function *F = M.getFunction(Name))
    return F;
  llvm::Function *F =
      llvm::Function::Create(Ty, llvm::GlobalValue::ExternalLinkage, Name, M);
  F->setDoesNotThrow();
  if (NoReturn)
    F->setDoesNotReturn();
  return F;
}

llvm::Value *CodeGenFunction::emitContractShadowCondition(
    const ContractStmt &S, const Expr *CallSite, const FunctionDecl *Callee,
    const ContractOverridePair *Override, int Tag) {
  CallerSideBinding Binding(*this, LocalDeclMap, CXXThisValue,
                            CXXThisAlignment);
  OverrideBinding BaseBinding(*this, LocalDeclMap, CXXThisValue,
                              CXXThisAlignment);
  if (Override) {
    if (isOverrideBaseContract(S, *Override)) {
      const ContractOverrideSnapshot *Snap = nullptr;
      if (ContractShadow) {
        auto It = ContractShadow->Snapshots.find(Override);
        if (It != ContractShadow->Snapshots.end())
          Snap = &It->second;
      }
      BaseBinding.bind(S.getCond(), *Override, Snap);
    }
  } else if (CallSite) {
    Binding.bind(S.getCond(), CallSite, Callee);
  }
  ContractShadowFunction Local;
  ContractShadowFunction *SavedState = ContractShadow;
  if (!ContractShadow)
    ContractShadow = &Local;
  int SavedTag = ContractShadow->PredicateTag;
  ContractShadow->PredicateTag = Tag;
  llvm::Value *Pred = EmitScalarExpr(S.getCond());
  if (Pred->getType() != Builder.getInt1Ty())
    Pred = Builder.CreateIsNotNull(Pred, "contract.tobool");
  ContractShadow->PredicateTag = SavedTag;
  ContractShadow = SavedState;
  return Pred;
}

void CodeGenFunction::emitContractShadowUnreachableUnless(llvm::Value *Pred) {
  llvm::BasicBlock *Cont = createBasicBlock("contract.proof.assumed");
  llvm::BasicBlock *Dead = createBasicBlock("contract.proof.unreachable");
  Builder.CreateCondBr(Pred, Cont, Dead);
  EmitBlock(Dead);
  Builder.CreateUnreachable();
  EmitBlock(Cont);
}

void CodeGenFunction::emitContractShadowPremise(unsigned Id,
                                                const ContractStmt &S,
                                                const Expr *CallSite,
                                                const FunctionDecl *Callee) {
  EnsureInsertPoint();
  emitContractShadowUnreachableUnless(
      emitContractShadowCondition(S, CallSite, Callee, nullptr, Id));
  if (!llvm::is_contained(ContractShadow->Relied, Id))
    ContractShadow->Relied.push_back(Id);
}

void CodeGenFunction::emitContractShadowPredicate(
    unsigned Id, const ContractStmt &S, const Expr *CallSite,
    const FunctionDecl *Callee, const ContractOverridePair *Override) {
  bool InShadow = ContractShadow && ContractShadow->InShadow;
  EnsureInsertPoint();
  llvm::BasicBlock *Cont = createBasicBlock("contract.proof.cont");
  if (!InShadow) {
    llvm::Function *Proven =
        getMarker(CGM, cs::ProvenFn,
                  llvm::FunctionType::get(Builder.getInt1Ty(),
                                          {Builder.getInt32Ty()}, false),
                  /*NoReturn=*/false);
    llvm::Value *Kept = Builder.CreateCall(Proven, {Builder.getInt32(Id)});
    llvm::BasicBlock *Region = createBasicBlock("contract.proof.assume");
    Builder.CreateCondBr(Kept, Region, Cont);
    EmitBlock(Region);
  }

  llvm::Value *Pred =
      emitContractShadowCondition(S, CallSite, Callee, Override, Id);

  if (InShadow) {
    llvm::BasicBlock *Fail = createBasicBlock("contract.proof.unproven");
    Builder.CreateCondBr(Pred, Cont, Fail);
    EmitBlock(Fail);
    llvm::Function *Unproven =
        getMarker(CGM, cs::UnprovenFn,
                  llvm::FunctionType::get(Builder.getVoidTy(),
                                          {Builder.getInt32Ty()}, false),
                  /*NoReturn=*/true);
    llvm::CallInst *Call = Builder.CreateCall(Unproven, {Builder.getInt32(Id)});
    Call->setDoesNotReturn();
    Call->setDoesNotThrow();
    Builder.CreateUnreachable();
  } else {
    llvm::CallInst *Assume = Builder.CreateAssumption(Pred);
    Assume->setMetadata(cs::AssumeMD, llvm::MDNode::get(getLLVMContext(), {}));
    Builder.CreateBr(Cont);
  }
  EmitBlock(Cont);
}

/// Whether no TU can continue past a violation of \p S: its allowed set
/// (the label's, which every TU sees in the declaration) holds none of
/// ignore, observe, noexcept_observe, diagnose and prove -- only terminating
/// semantics and assume.  Its evaluation is then a guaranteed premise
/// wherever its code runs, whichever TU's code that is.
bool CodeGen::isContractPremiseSettled(const ContractStmt &S) {
  using CES = ContractEvaluationSemantic;
  unsigned Mask = S.getAllowedMask();
  unsigned Continuing = 0;
  for (CES Sem : {CES::Ignore, CES::Observe, CES::NoexceptObserve,
                  CES::Diagnose, CES::Prove})
    Continuing |= 1u << static_cast<unsigned>(Sem);
  return Mask != 0 && !(Mask & Continuing);
}

/// List the function being emitted, once, in the named metadata \p MD
/// (\p Names: those already listed); ValueAsMetadata, so the entry follows
/// a replaced function.
static void listContractFunction(CodeGenFunction &CGF, llvm::StringRef MD,
                                 llvm::StringSet<> &Names) {
  if (!Names.insert(CGF.CurFn->getName()).second)
    return;
  llvm::NamedMDNode *List = CGF.CGM.getModule().getOrInsertNamedMetadata(MD);
  List->addOperand(llvm::MDNode::get(CGF.getLLVMContext(),
                                     llvm::ValueAsMetadata::get(CGF.CurFn)));
}

/// Note that the function being emitted holds a terminating check of \p S
/// that only this TU's copy of it decides (the spec's "premises decided by
/// this TU"): unless rule 2 settles it, the back end may rely on it through
/// that copy only where the real code uses that copy.
static void noteContractPremise(CodeGenFunction &CGF, const ContractStmt &S,
                                ContractEvaluationSemantic Sem) {
  using CES = ContractEvaluationSemantic;
  if ((Sem != CES::Enforce && Sem != CES::NoexceptEnforce &&
       Sem != CES::QuickEnforce) ||
      !CGF.CurFn || !isContractShadowActive(CGF.CGM) ||
      isContractPremiseSettled(S))
    return;
  listContractFunction(CGF, cs::PremisesMD,
                       CGF.CGM.getContractShadowState().PremiseNames);
}

void CodeGenFunction::noteContractRefinableBody() {
  if ((ContractShadow && ContractShadow->InShadow) || !CurFn ||
      !isContractShadowActive(CGM))
    return;
  listContractFunction(*this, cs::RefinableMD,
                       CGM.getContractShadowState().RefinableNames);
}

bool CodeGenFunction::emitContractShadowAssumption(
    const ContractStmt &S, ContractEvaluationSemantic Sem, bool Dynamic) {
  if (!ContractShadow || !ContractShadow->InShadow) {
    noteContractPremise(*this, S, Sem);
    return false;
  }
  if (Dynamic || S.isInvalid())
    return false;
  // D4405: an override shadow's obligation of the assertion precedes it, as
  // an analysis evaluation precedes the runtime one in the body.
  if (ContractShadow->Override &&
      ContractShadow->OverrideObligations.contains(&S))
    return false;
  bool Guaranteed, Evaluated;
  switch (Sem) {
  case ContractEvaluationSemantic::Assume:
    Guaranteed = true;
    Evaluated = false;
    break;
  case ContractEvaluationSemantic::Ignore:
    Guaranteed = false;
    Evaluated = false;
    break;
  case ContractEvaluationSemantic::Observe:
  case ContractEvaluationSemantic::NoexceptObserve:
    Guaranteed = false;
    Evaluated = true;
    break;
  default:
    // enforce, noexcept_enforce, quick_enforce: the check itself, whose
    // violation path does not return (a runtime evaluation is never prove
    // or diagnose, D4405: the analysis evaluation is an obligation).
    return false;
  }
  // An evaluation that does not evaluate its predicate assumes it only if it
  // has no side effects; otherwise nothing is emitted, as in the real code.
  if (!Evaluated && S.getCond()->HasSideEffects(getContext()))
    return false;
  EnsureInsertPoint();
  llvm::Value *Pred = EmitScalarExpr(S.getCond());
  if (Pred->getType() != Builder.getInt1Ty())
    Pred = Builder.CreateIsNotNull(Pred, "contract.tobool");
  emitContractShadowUnreachableUnless(Pred);
  if (!Guaranteed)
    ContractShadow->UnguaranteedAssumption = true;
  return true;
}

void CodeGenModule::emitContractProofShadow(GlobalDecl GD, StringRef Name,
                                            unsigned Pair) {
  llvm::Function *Real = getModule().getFunction(Name);
  if (!Real || Real->isDeclaration())
    return;
  const CGFunctionInfo &FI = getTypes().arrangeGlobalDeclaration(GD);
  // D4405: an override shadow, "<name>.contract.override.<pair>".
  std::string ShadowName =
      Pair ? (Name + ".contract.override." + llvm::Twine(Pair)).str()
           : (Name + ".contract.proof").str();
  llvm::Function *Shadow = llvm::Function::Create(
      Real->getFunctionType(), llvm::GlobalValue::InternalLinkage,
      Real->getAddressSpace(), ShadowName, &getModule());
  Shadow->copyAttributesFrom(Real);
  Shadow->setLinkage(llvm::GlobalValue::InternalLinkage);
  Shadow->setVisibility(llvm::GlobalValue::DefaultVisibility);
  Shadow->setDLLStorageClass(llvm::GlobalValue::DefaultStorageClass);
  Shadow->setComdat(nullptr);
  // D4405: an indirect return slot is the result object, which exists, so a
  // precondition naming `this' of a constructor whose temporary is elided
  // into it (`return T (y);`) can be proven.  The real function does not
  // mark it nonnull; here the attribute informs only the proofs.
  if (!getCodeGenOpts().NullPointerIsValid)
    for (llvm::Argument &A : Shadow->args())
      if (A.hasStructRetAttr())
        A.addAttr(llvm::Attribute::NonNull);

  ContractShadowFunction State;
  State.InShadow = true;
  if (Pair)
    State.Override = &getContractShadowState().OverridePairs[Pair - 1];
  DiagnosticsEngine &Diags = getDiags();
  bool SavedSuppress = Diags.getSuppressAllDiagnostics();
  Diags.setSuppressAllDiagnostics(true);
  {
    CodeGenFunction CGF(*this);
    CGF.disableDebugInfo();
    CGF.ContractShadow = &State;
    CGF.GenerateCode(GD, Shadow, FI);
  }
  Diags.setSuppressAllDiagnostics(SavedSuppress);
  Shadow->addFnAttr(cs::ShadowAttr,
                    State.UnguaranteedAssumption ? "conditional" : "");
  if (!Pair)
    return;
  Shadow->addFnAttr(cs::OverrideAttr, llvm::utostr(Pair));
  llvm::SmallVector<llvm::Metadata *, 4> Relied;
  for (unsigned Id : State.Relied)
    Relied.push_back(llvm::ConstantAsMetadata::get(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(getLLVMContext()), Id)));
  Shadow->setMetadata(cs::ReliedMD,
                      llvm::MDTuple::get(getLLVMContext(), Relied));
}

void CodeGenModule::emitContractProofShadows() {
  if (!ContractShadowData)
    return;
  ContractShadowModuleState &St = *ContractShadowData;
  // A shadow can make code generation emit more functions (those only a
  // predicate uses), which may defer evaluations of their own.
  while (St.NextPending < St.Pending.size()) {
    while (St.NextPending < St.Pending.size()) {
      auto [GD, Name, Pair] = St.Pending[St.NextPending++];
      emitContractProofShadow(GD, Name, Pair);
    }
    EmitDeferred();
  }
}

void CodeGenModule::reportContractProofsNow() {
  if (!ContractShadowData)
    return;
  for (const ContractObligation &O : ContractShadowData->Obligations) {
    SourceLocation Loc;
    SourceRange Range, PredRange;
    getObligationLocations(O, Loc, Range, PredRange);
    reportContractObligation(
        getDiags(), O.Flags, Loc, Range, PredRange,
        O.Decision == cs::TakenAlwaysFalse ? ContractProofResult::AlwaysFalse
                                           : ContractProofResult::Unproven,
        &getContext(), O.DiagContext,
        O.Override ? StringRef(O.Override->BaseName) : StringRef());
  }
  ContractShadowData->Obligations.clear();
}

void CodeGen::reportContractObligation(
    DiagnosticsEngine &Diags, unsigned Flags, SourceLocation Loc,
    SourceRange Range, SourceRange PredRange, ContractProofResult R,
    const ASTContext *Ctx, unsigned DiagContext, StringRef OverrideName) {
  bool AlwaysFalse = R == ContractProofResult::AlwaysFalse;
  unsigned ID = (Flags & cs::Prove)
                    ? (AlwaysFalse ? diag::err_contract_always_false
                                   : diag::err_contract_unproven)
                    : (AlwaysFalse ? diag::warn_contract_always_false
                                   : diag::warn_contract_unproven);
  // D4405: an override pair's is at D::f's definition, naming B::f.
  bool Override = Flags & cs::Override;
  if (!(Flags & cs::CallerSide)) {
    Diags.Report(Loc, ID) << PredRange << Override << OverrideName;
    if (Ctx)
      Ctx->emitContractDiagnosticContext(Diags, DiagContext);
    return;
  }
  Diags.Report(Loc, ID) << Range << Override << OverrideName;
  if (Ctx)
    Ctx->emitContractDiagnosticContext(Diags, DiagContext);
  // The note follows the diagnostic, so it is dropped with a silenced warning.
  Diags.Report(PredRange.getBegin(), diag::note_contract_unproven_predicate)
      << PredRange;
}
