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
// When the strategy is on (from proof level 1 up, D4405's
// -fcontracts-prove-optimize=, at any -O level; and none of the exclusions
// in isContractShadowActive), an evaluation with prove
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
// An -O0 module proving at a proof level above 0 (contractProofsAtO0) makes
// what the proof level needs and -O0 does not, for the proofs only: TBAA and
// !range/!noundef everywhere, which the back end strips from the real code
// (O0MD); the bodies of available_externally functions, emitted with the
// shadows and then moved into "<name>.contract.external" (ExternalAttr),
// the real function staying a declaration; and a list of what the proof
// level does to each function's attributes that -O0 does not (optnone,
// noinline, inlinehint, optsize), which the proof module applies.  The
// allocas made for an assumption region are marked (RegionTempMD), so that
// the back end deletes those it leaves unused, which -O0 would keep.
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
// back end runs makes reportContractProofsNow diagnose them instead.  An
// evaluation at a call in code that code generation does not emit
// (-fno-contract-analysis-skip-dead-calls, CodeGenFunction::
// analyzeContractDeadCalls) is never deferred: there is no code to defer.
//
// The evaluations nested in an obligation (DECISIONS.md E169) are placed in
// its shadow as its predicate is emitted, at each call there
// (emitContractShadowNested): folded first, in the frame of the call, whose
// arguments fold in the frame of the predicate holding it; recorded with
// the result if folding decides it (and it has no nested evaluations of its
// own) or if it cannot be deferred -- a constructor call, or arguments that
// cannot be given again -- with those nested in it decided by folding too;
// otherwise a nested obligation, its region a side branch ahead of the call
// (see ContractProveShadow.h).  In a real function's assumption nothing is
// placed at the calls: the predicate is never evaluated there, and an
// evaluation at a call that the program does make must not be taken for
// one made in an assumption.  An evaluation decided at once with nested
// evaluations is recorded with them, decided at once too.
//
//===----------------------------------------------------------------------===//

#include "ContractProveShadow.h"
#include "CGDebugInfo.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/Attr.h"
#include "clang/AST/ContractFold.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/CodeGenOptions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include <optional>
#include <set>
#include <tuple>

using namespace clang;
using namespace CodeGen;
namespace cs = contract_shadow;

//===----------------------------------------------------------------------===//
// When shadows are made
//===----------------------------------------------------------------------===//

/// Whether nothing in the options rules proof shadows out: nothing would
/// duplicate state a second emission of a function cannot share (front-end
/// profile instrumentation or use, coverage mapping), stream the shadows'
/// optimization remarks to a file, or lower functions through another
/// runtime (OpenMP, CUDA/HIP, OpenCL, SYCL, HLSL, the incremental compiler).
static bool contractShadowOptionsAllow(const LangOptions &LO,
                                       const CodeGenOptions &CO) {
  return !LO.OpenMP && !LO.CUDA && !LO.OpenCL && !LO.SYCLIsDevice &&
         !LO.HLSL && !LO.IncrementalExtensions && !CO.hasProfileClangInstr() &&
         !CO.hasProfileClangUse() && !CO.CoverageMapping &&
         CO.OptRecordFile.empty();
}

/// Whether proof shadows, of any kind, can be made in this module:
/// contractShadowOptionsAllow.  At any -O level (D4405: the proofs are
/// optimized at the proof level, -fcontracts-prove-optimize=; -O0 too, and
/// with the LLVM passes disabled, as -save-temps does for the step that
/// writes the bitcode); the strategies are on from their proof levels up.
bool CodeGen::canMakeContractShadows(CodeGenModule &CGM) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  if (St.CanShadow >= 0)
    return St.CanShadow;
  bool Can = contractShadowOptionsAllow(CGM.getLangOpts(), CGM.getCodeGenOpts());
  St.CanShadow = Can;
  return Can;
}

bool CodeGen::contractProofsAtO0(const LangOptions &LO,
                                 const CodeGenOptions &CO) {
  return CO.OptimizationLevel == 0 && LO.ContractsP4405 &&
         LO.ContractsProveOptimize > 0 &&
         (LO.ContractsProveShadow || LO.ContractsProveOverride) &&
         contractShadowOptionsAllow(LO, CO);
}

bool CodeGen::contractProofTBAAAtO0(const LangOptions &LO,
                                    const CodeGenOptions &CO) {
  return contractProofsAtO0(LO, CO) && !CO.RelaxedAliasing &&
         !LO.Sanitize.hasOneOf(SanitizerKind::Thread | SanitizerKind::Type);
}

void CodeGenModule::initContractProofsAtO0() {
  ContractProofsAtO0 = contractProofsAtO0(LangOpts, CodeGenOpts);
  ContractProofTBAA = contractProofTBAAAtO0(LangOpts, CodeGenOpts);
}

/// D4405: in an -O0 module proving at a proof level above 0, an
/// available_externally function's body is emitted as at -O1, for the
/// proofs only: those met while the real code is emitted are noted, and
/// emitted with the shadows (emitContractProofShadows); those met then are
/// emitted at once.  Either way, its name is noted: its body is moved out of
/// the real function, which stays a declaration.
bool CodeGenModule::shouldEmitContractExternalBody(GlobalDecl GD) {
  if (!ContractProofsAtO0)
    return false;
  ContractShadowModuleState &St = getContractShadowState();
  if (!ContractProofEmission) {
    if (St.ExternalBodySet.insert(GD).second)
      St.ExternalBodies.push_back(GD);
    return false;
  }
  StringRef Name = getMangledName(GD);
  if (St.ExternalNameSet.insert(Name).second)
    St.ExternalNames.push_back(Name.str());
  return true;
}

/// Whether proof shadows are made in this module at all: the flag, and
/// canMakeContractShadows.
bool CodeGen::isContractShadowActive(CodeGenModule &CGM) {
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
      llvm::MDString::get(Ctx, O.Override ? O.Override->BaseName : ""),
      Field(O.Parent + 1),
      Field(O.Group)};
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
                                        unsigned Decision, int Parent) {
  ContractShadowModuleState &St = CGM.getContractShadowState();
  unsigned Id = St.Obligations.size();
  ContractObligation O{&Q.Contract, Q.CallSite, Q.Callee, Q.Semantic};
  if (Q.Semantic == ContractEvaluationSemantic::Prove)
    O.Flags |= cs::Prove;
  if (Q.CallSite)
    O.Flags |= cs::CallerSide;
  if (Q.CGF && Q.CGF->isInContractShadowLoop())
    O.Flags |= cs::InLoop;
  if (Q.FoldedFalse)
    O.Flags |= cs::FoldedFalse;
  if (Q.Override) {
    O.Flags |= cs::Override;
    if (Q.OverrideShadow)
      O.Flags |= cs::OverrideShadow;
    O.Override = Q.Override;
  }
  O.Decision = Decision;
  O.DiagContext = CGM.getContext().getContractDiagnosticContext(
      Q.CGF ? Q.CGF->CurCodeDecl : nullptr);
  O.Parent = Parent;
  if (Parent >= 0) {
    // DECISIONS.md E169: a nested evaluation is in the place, and for an
    // override pair's (D4405), the pair, of the one it is nested in.
    const ContractObligation &P = St.Obligations[Parent];
    O.Group = P.Group;
    if (P.Override) {
      O.Override = P.Override;
      O.Flags |= P.Flags & (cs::Override | cs::OverrideShadow);
    }
  } else {
    // E169 (9), (11): an evaluation at a call made in a check is in the
    // check's place; any other is in its own.
    std::pair<const ContractStmt *, const Expr *> Place{&Q.Contract,
                                                        Q.CallSite};
    if (Q.CallSite && Q.CGF)
      Q.CGF->getContractCheckPlace(Place.first, Place.second);
    unsigned &G = St.GroupIds[Place];
    if (!G)
      G = St.GroupIds.size();
    O.Group = G;
  }
  St.Obligations.push_back(O);
  if (Parent < 0)
    St.ObligationIds.insert({{&Q.Contract, Q.CallSite}, Id});
  else
    St.NestedIds.insert(
        {{&Q.Contract, Q.CallSite, static_cast<unsigned>(Parent)}, Id});
  writeObligation(CGM, Id);
  return Id;
}

/// The decision recorded for an evaluation decided \p R at once.
static unsigned takenDecision(ContractProofResult R) {
  return R == ContractProofResult::AlwaysFalse ? cs::TakenAlwaysFalse
         : isContractProofProven(R)            ? cs::TakenSound
                                               : cs::TakenUnproven;
}

static unsigned takenDecision(ContractFoldResult R) {
  return R == ContractFoldResult::False  ? cs::TakenAlwaysFalse
         : R == ContractFoldResult::True ? cs::TakenSound
                                         : cs::TakenUnproven;
}

/// Record \p Nested, evaluations decided at once (by folding) that are
/// nested, directly or not, in obligation \p Parent; \p Q is \p Parent's
/// query.
static void recordNestedDecided(CodeGenModule &CGM, const ContractProofQuery &Q,
                                ArrayRef<ContractNestedEvaluation> Nested,
                                unsigned Parent) {
  SmallVector<unsigned, 4> Ids;
  for (const ContractNestedEvaluation &N : Nested) {
    ContractProofQuery NQ{
        Q.Ctx,      *N.Contract, N.Semantic, N.Contract->getCond(),
        N.CallSite, N.Callee,    Q.CGF};
    Ids.push_back(addContractObligation(CGM, NQ, takenDecision(N.Result),
                                        N.Parent < 0 ? static_cast<int>(Parent)
                                                     : Ids[N.Parent]));
  }
}

unsigned CodeGen::addTakenContractObligation(
    CodeGenModule &CGM, const ContractProofQuery &Q, ContractProofResult R,
    ArrayRef<ContractNestedEvaluation> Nested) {
  unsigned Id = addContractObligation(CGM, Q, takenDecision(R));
  recordNestedDecided(CGM, Q, Nested, Id);
  return Id;
}

void CodeGen::combineNestedContractResults(
    ArrayRef<int> Parent, ArrayRef<bool> Prove,
    ArrayRef<ContractProofResult> Raw,
    SmallVectorImpl<ContractProofResult> &Effective,
    SmallVectorImpl<SmallVector<unsigned, 2>> &Causes) {
  size_t N = Raw.size();
  Effective.assign(Raw.begin(), Raw.end());
  Causes.assign(N, {});
  SmallVector<bool, 16> Conditional(N, false);
  // Each nested obligation counts for every obligation above it.
  for (size_t D = 0; D != N; ++D) {
    if (Parent[D] < 0)
      continue;
    ContractProofResult R = Raw[D];
    bool Fails = !isContractProofProven(R);
    bool Doubtful = Prove[D] ? R == ContractProofResult::Conditional
                             : R != ContractProofResult::Sound;
    for (int A = Parent[D]; A >= 0 && static_cast<size_t>(A) < N;
         A = Parent[A]) {
      if (Prove[D] && Fails)
        Causes[A].push_back(D);
      else if (Doubtful)
        Conditional[A] = true;
    }
  }
  for (size_t I = 0; I != N; ++I) {
    if (!isContractProofProven(Raw[I]))
      continue;
    if (!Causes[I].empty())
      Effective[I] = ContractProofResult::Unproven;
    else if (Conditional[I])
      Effective[I] = ContractProofResult::Conditional;
  }
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
/// preconditions must then fold (DECISIONS.md E157).  Never one at a call
/// code generation does not emit (CodeGenFunction::analyzeContractDeadCalls),
/// which has no code to defer to.
static ContractProofResult proveByShadow(const ContractProofQuery &Q) {
  CodeGenFunction *CGF = Q.CGF;
  if (!CGF || CGF->ContractShadow || CGF->InContractDeadCode ||
      !isContractShadowActive(CGF->CGM) || !CGF->isContractShadowable() ||
      Q.Predicate->isValueDependent() || Q.Contract.hasCaptures())
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
  // D4405: a premise copy holds no obligation (and diagnoses nothing): the
  // real function has placed or decided each.
  if (CGF && CGF->ContractShadow && CGF->ContractShadow->PremiseCopy)
    return true;
  // An override shadow's obligation is the override strategy's; code not
  // emitted has nowhere to place one.
  if (!CGF || CGF->InContractDeadCode || (Q.Override && Q.OverrideShadow))
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

/// While shadows are made, take every diagnostic decided at once, with the
/// evaluations nested in it decided at once too (DECISIONS.md E169), so that
/// the back end issues it in order with the deferred ones.  One met while a
/// shadow is emitted is dropped: an evaluation the real code emits has been
/// decided already.
static bool takeShadowDiagnostic(const ContractProofQuery &Q,
                                 ContractProofResult R,
                                 ArrayRef<ContractNestedEvaluation> Nested) {
  if (!Q.CGF || !isContractShadowActive(Q.CGF->CGM))
    return false;
  if (Q.CGF->ContractShadow)
    return true;
  addTakenContractObligation(Q.CGF->CGM, Q, R, Nested);
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

/// DECISIONS.md E169: in a proof shadow, emit the region of nested
/// obligation \p Id, the evaluation of the precondition \p S at the call
/// \p CallSite of \p Callee, ahead of the call: a side branch, behind a guard
/// the optimizers know nothing of, that never rejoins the path -- `if
/// (__clang_contract_nested(id)) { if (!pred) __clang_contract_unproven(id);
/// llvm.trap(); }` -- so that it is decided where the call is evaluated,
/// before anything the callee does, and is no premise for what follows.
/// The evaluations at the calls in its predicate are nested in it, with
/// \p Chain and \p Frame (its call's frame).
static void emitNestedRegion(CodeGenFunction &CGF, unsigned Id,
                             const ContractStmt &S, const Expr *CallSite,
                             const FunctionDecl *Callee,
                             ArrayRef<const ContractStmt *> Chain,
                             const ContractFoldFrame &Frame) {
  ContractShadowFunction &St = *CGF.ContractShadow;
  CGBuilderTy &B = CGF.Builder;
  CodeGenModule &CGM = CGF.CGM;
  CGF.EnsureInsertPoint();
  // The guard, the trap and the marker are not the predicate's: untagged.
  int SavedTag = St.PredicateTag;
  St.PredicateTag = -1;
  llvm::Function *Guard = CGM.getModule().getFunction(cs::NestedFn);
  if (!Guard) {
    Guard = llvm::Function::Create(
        llvm::FunctionType::get(B.getInt1Ty(), {B.getInt32Ty()}, false),
        llvm::GlobalValue::ExternalLinkage, cs::NestedFn, CGM.getModule());
    // Opaque, yet it neither throws nor fails to return, and it touches no
    // memory the program can see; two calls are never merged.
    Guard->setDoesNotThrow();
    Guard->setWillReturn();
    Guard->setOnlyAccessesInaccessibleMemory();
  }
  llvm::Value *Taken = B.CreateCall(Guard, {B.getInt32(Id)});
  llvm::BasicBlock *Region = CGF.createBasicBlock("contract.proof.nested");
  llvm::BasicBlock *Cont = CGF.createBasicBlock("contract.proof.nested.cont");
  B.CreateCondBr(Taken, Region, Cont);
  CGF.EmitBlock(Region);
  St.PredicateTag = SavedTag;

  int SavedParent = St.NestParent;
  const ContractFoldFrame *SavedFrame = St.NestFrame;
  llvm::SmallVector<const ContractStmt *, 4> SavedChain =
      std::move(St.NestChain);
  St.NestParent = Id;
  St.NestChain.assign(Chain.begin(), Chain.end());
  St.NestFrame = &Frame;
  llvm::Value *Pred;
  {
    CodeGenFunction::RunCleanupsScope Scope(CGF);
    Pred = CGF.emitContractShadowCondition(S, CallSite, Callee, nullptr, Id);
    Scope.ForceCleanup({&Pred});
  }
  St.NestParent = SavedParent;
  St.NestChain = std::move(SavedChain);
  St.NestFrame = SavedFrame;

  SavedTag = St.PredicateTag;
  St.PredicateTag = -1;
  llvm::BasicBlock *Holds = CGF.createBasicBlock("contract.proof.nested.end");
  llvm::BasicBlock *Fail = CGF.createBasicBlock("contract.proof.unproven");
  B.CreateCondBr(Pred, Holds, Fail);
  CGF.EmitBlock(Fail);
  llvm::Function *Unproven =
      getMarker(CGM, cs::UnprovenFn,
                llvm::FunctionType::get(B.getVoidTy(), {B.getInt32Ty()}, false),
                /*NoReturn=*/true);
  llvm::CallInst *Call = B.CreateCall(Unproven, {B.getInt32(Id)});
  Call->setDoesNotReturn();
  Call->setDoesNotThrow();
  B.CreateUnreachable();
  CGF.EmitBlock(Holds);
  // A trap, not `unreachable': the predicate holding there must not be taken
  // for undefined behavior, which would make the marker unconditional.
  llvm::CallInst *Trap = B.CreateCall(CGM.getIntrinsic(llvm::Intrinsic::trap));
  Trap->setDoesNotReturn();
  Trap->setDoesNotThrow();
  B.CreateUnreachable();
  St.PredicateTag = SavedTag;
  CGF.EmitBlock(Cont);
}

/// DECISIONS.md E169: place the evaluation of the precondition \p S with
/// \p Sem at the call \p CallSite of \p Callee, nested in the obligation
/// whose predicate is being emitted in \p CGF's proof shadow.
static void placeNestedEvaluation(CodeGenFunction &CGF, const ContractStmt &S,
                                  ContractEvaluationSemantic Sem,
                                  const Expr *CallSite,
                                  const FunctionDecl *Callee) {
  CodeGenModule &CGM = CGF.CGM;
  ASTContext &Ctx = CGM.getContext();
  ContractShadowFunction &St = *CGF.ContractShadow;
  ContractShadowModuleState &MS = CGM.getContractShadowState();
  unsigned Parent = St.NestParent;
  ContractProofQuery Q{Ctx, S, Sem, S.getCond(), CallSite, Callee, &CGF};
  Q.OuterFrame = St.NestFrame;
  ContractFoldFrame Frame(Ctx, S.getCond(), CallSite, Callee, St.NestFrame);
  llvm::SmallVector<const ContractStmt *, 8> Chain(St.NestChain.begin(),
                                                   St.NestChain.end());
  Chain.push_back(&S);

  auto It = MS.NestedIds.find({&S, CallSite, Parent});
  if (It != MS.NestedIds.end()) {
    // Another copy of the code (a constructor's other variant, say).
    if (MS.Obligations[It->second].Decision == cs::Deferred)
      emitNestedRegion(CGF, It->second, S, CallSite, Callee, Chain, Frame);
    return;
  }

  bool Fold = CGM.getLangOpts().ContractsProveFold;
  ContractFoldResult R = Fold ? foldContractPredicate(Ctx, S.getCond(), Frame)
                              : ContractFoldResult::Unknown;
  bool HasNested = hasNestedContractEvaluations(Ctx, S.getCond(),
                                                Fold ? &Frame : nullptr, Chain);
  if (R == ContractFoldResult::True && !HasNested)
    return;
  // Deferred only at a call (not a constructor's) whose arguments can be
  // given again, as for a caller-side obligation.
  bool CanDefer = R != ContractFoldResult::False && isa<CallExpr>(CallSite) &&
                  !isa<CXXConstructorDecl, CXXDestructorDecl>(Callee) &&
                  canSubstituteCallerSide(Q);
  if (CanDefer) {
    unsigned Id = addContractObligation(CGM, Q, cs::Deferred, Parent);
    emitNestedRegion(CGF, Id, S, CallSite, Callee, Chain, Frame);
    return;
  }
  // Decided at once, by folding, with those nested in it.
  unsigned Id = addContractObligation(CGM, Q, takenDecision(R), Parent);
  SmallVector<ContractNestedEvaluation, 4> Nested;
  decideNestedContractEvaluations(Ctx, S.getCond(), &Frame, Fold, Nested,
                                  /*Parent=*/-1, Chain);
  recordNestedDecided(CGM, Q, Nested, Id);
}

bool CodeGenFunction::emitContractShadowNested(const FunctionDecl *FD,
                                               const Expr *CallSite) {
  // A real function's assumption: its predicate is never evaluated, and
  // nothing is decided at its calls.
  if (!ContractShadow->InShadow)
    return true;
  if (ContractShadow->NestParent < 0)
    return false;
  // In an obligation's predicate in a proof shadow: the analysis evaluations
  // of the callee's preconditions, nested in it.  The callee's other
  // assertions are analyzed in its body (DECISIONS.md E169 (1)), and a
  // destructor's preconditions are not nested.
  const FunctionDecl *WithContracts;
  const FunctionDecl *Callee = getContractEvaluatedCallee(FD, WithContracts);
  if (!WithContracts || isa<CXXDestructorDecl>(Callee) ||
      !getLangOpts().CPlusPlus)
    return true;
  for (const ContractStmt *CS : WithContracts->getContracts()->contracts()) {
    if (CS->getContractKind() != ContractKind::Pre || CS->isInvalid() ||
        CS->getCond()->isValueDependent() ||
        !canNestContractEvaluation(*CS, ContractShadow->NestChain))
      continue;
    ContractEvaluationSemantic Sem =
        getContractCallAnalysisSemantic(getContext(), *CS, WithContracts);
    if (isProvingContractSemantic(Sem))
      placeNestedEvaluation(*this, *CS, Sem, CallSite, Callee);
  }
  return true;
}

void CodeGenFunction::emitContractShadowPredicate(
    unsigned Id, const ContractStmt &S, const Expr *CallSite,
    const FunctionDecl *Callee, const ContractOverridePair *Override) {
  bool InShadow = ContractShadow && ContractShadow->InShadow;
  EnsureInsertPoint();
  llvm::BasicBlock *Cont = createBasicBlock("contract.proof.cont");
  // D4405: the allocas made for a real function's assumption region are
  // marked, so that the back end deletes those it leaves unused with the
  // region (-O0 would keep them; RegionTempMD).
  llvm::Instruction *AllocaMark =
      InShadow ? nullptr : AllocaInsertPt->getPrevNode();
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

  // DECISIONS.md E169: in a shadow, the evaluations at the calls in the
  // predicate are nested in this obligation -- an override pair's too, in
  // its override shadow or, without one, in D::f's proof shadow (E169 (8)).
  ContractShadowFunction *St = ContractShadow;
  bool Nests = InShadow;
  std::optional<ContractFoldFrame> Frame;
  int SavedParent = Nests ? St->NestParent : -1;
  const ContractFoldFrame *SavedFrame = Nests ? St->NestFrame : nullptr;
  llvm::SmallVector<const ContractStmt *, 4> SavedChain;
  if (Nests) {
    Frame.emplace(getContext(), S.getCond(), CallSite, Callee,
                  /*Outer=*/nullptr);
    SavedChain = std::move(St->NestChain);
    St->NestParent = Id;
    St->NestChain.assign({&S});
    St->NestFrame = &*Frame;
  }
  llvm::Value *Pred =
      emitContractShadowCondition(S, CallSite, Callee, Override, Id);
  if (Nests) {
    St->NestParent = SavedParent;
    St->NestChain = std::move(SavedChain);
    St->NestFrame = SavedFrame;
  }

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
    llvm::BasicBlock &Entry = CurFn->getEntryBlock();
    for (llvm::Instruction *I = AllocaMark ? AllocaMark->getNextNode()
                                           : &Entry.front();
         I && I != AllocaInsertPt; I = I->getNextNode())
      if (isa<llvm::AllocaInst>(I))
        I->setMetadata(cs::RegionTempMD,
                       llvm::MDNode::get(getLLVMContext(), {}));
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
  if ((ContractShadow &&
       (ContractShadow->InShadow || ContractShadow->PremiseCopy)) ||
      !CurFn ||
      !isContractShadowActive(CGM))
    return;
  listContractFunction(*this, cs::RefinableMD,
                       CGM.getContractShadowState().RefinableNames);
}

bool CodeGenFunction::emitContractShadowAssumption(
    const ContractStmt &S, ContractEvaluationSemantic Sem, bool Dynamic) {
  if (!ContractShadow ||
      (!ContractShadow->InShadow && !ContractShadow->PremiseCopy)) {
    noteContractPremise(*this, S, Sem);
    noteContractPremiseCopy(S, Sem, Dynamic);
    return false;
  }
  // A postcondition with captures (P3098) assumes nothing: its captures are
  // emitted only where it is checked (as proveByShadow defers none).
  if (Dynamic || S.isInvalid() || S.hasCaptures())
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
  // D4405, "Previous Assertions and Undefined Behavior": an evaluation that
  // does not evaluate its predicate assumes it unless it has side effects of
  // its own (then nothing is emitted, as in the real code).  The calls it
  // makes are tagged (PremiseTag): a proof resting on it is conditional
  // unless each function called is found free of side effects, returning and
  // not throwing (ContractShadowDecide.cpp), as an obligation's callees are.
  if (!Evaluated &&
      S.getCond()->HasSideEffects(getContext(),
                                  /*IncludePossibleEffects=*/false))
    return false;
  EnsureInsertPoint();
  llvm::Value *Pred;
  if (Evaluated || !ContractShadow->InShadow) {
    Pred = EmitScalarExpr(S.getCond());
    if (Pred->getType() != Builder.getInt1Ty())
      Pred = Builder.CreateIsNotNull(Pred, "contract.tobool");
  } else {
    Pred = emitContractShadowCondition(S, nullptr, nullptr, nullptr,
                                       cs::PremiseTag);
  }
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

/// D4405: whether \p FD, or the pattern it was instantiated from, is declared
/// inline (as SetLLVMFunctionAttributesForDefinition asks).
static bool isDeclaredInline(const FunctionDecl *FD) {
  auto Inline = [](const FunctionDecl *F) {
    return llvm::any_of(F->redecls(), [](const FunctionDecl *R) {
      return R->isInlineSpecified();
    });
  };
  if (Inline(FD))
    return true;
  const FunctionDecl *Pattern = FD->getTemplateInstantiationPattern();
  return Pattern && Inline(Pattern);
}

/// D4405: in an -O0 module proving at a proof level above 0, list in \p List
/// (O0MD) what the proof level does to each function's attributes that -O0
/// does not (SetLLVMFunctionAttributesForDefinition): no optnone but the
/// function's own; noinline only where its declaration or the inlining the
/// options ask for (-fno-inline, -fno-inline-functions,
/// -finline-hint-functions; CodeGenOptions::ContractProofInlining) gives it;
/// inlinehint for a function declared inline; optsize for a cold one.
static void listContractO0Attributes(CodeGenModule &CGM,
                                     llvm::NamedMDNode *List) {
  using llvm::Attribute;
  CodeGenOptions::InliningMethod Inlining = CGM.getCodeGenOpts().getContractProofInlining();
  llvm::LLVMContext &Ctx = CGM.getLLVMContext();
  for (llvm::Function &F : CGM.getModule()) {
    if (F.isDeclaration() || F.hasFnAttribute(cs::ShadowAttr) ||
        F.hasFnAttribute(cs::PremiseCopyAttr))
      continue;
    GlobalDecl GD;
    const Decl *D =
        CGM.lookupRepresentativeDecl(F.getName(), GD) ? GD.getDecl() : nullptr;
    const auto *FD = dyn_cast_or_null<FunctionDecl>(D);
    bool Always = F.hasFnAttribute(Attribute::AlwaysInline);
    bool OwnOptNone = D && D->hasAttr<OptimizeNoneAttr>();
    bool NoInline = false, Hint = false;
    if (!D)
      NoInline = !Always && Inlining == CodeGenOptions::OnlyAlwaysInlining;
    else if (OwnOptNone || D->hasAttr<NakedAttr>())
      NoInline = !Always || D->hasAttr<NakedAttr>();
    else if (D->hasAttr<NoDuplicateAttr>() || D->hasAttr<AlwaysInlineAttr>())
      NoInline = false;
    else if (D->hasAttr<NoInlineAttr>() ||
             Inlining == CodeGenOptions::OnlyAlwaysInlining)
      NoInline = !Always;
    else if (FD && isDeclaredInline(FD))
      Hint = true;
    else if (FD && Inlining == CodeGenOptions::OnlyHintInlining &&
             !FD->isInlined())
      NoInline = !Always;
    unsigned Bits = 0;
    if (F.hasFnAttribute(Attribute::OptimizeNone) && !OwnOptNone)
      Bits |= cs::O0OptNone;
    if (F.hasFnAttribute(Attribute::NoInline) && !NoInline)
      Bits |= cs::O0NoInline;
    if (Hint && !F.hasFnAttribute(Attribute::InlineHint))
      Bits |= cs::O0InlineHint;
    if (D && !OwnOptNone && D->hasAttr<ColdAttr>() &&
        !F.hasFnAttribute(Attribute::OptimizeForSize))
      Bits |= cs::O0OptSize;
    if (!Bits)
      continue;
    llvm::Metadata *Ops[] = {
        llvm::ValueAsMetadata::get(&F),
        llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), Bits))};
    List->addOperand(llvm::MDNode::get(Ctx, Ops));
  }
}

/// D4405: move the body of each available_externally function emitted for
/// the proofs only (\p Names) into "<name>.contract.external", and make the
/// function the declaration it was (\p Saved: a declaration holding its
/// attributes then, if it had one).
static void moveContractExternalBodies(
    CodeGenModule &CGM, ArrayRef<std::string> Names,
    llvm::StringMap<llvm::Function *> &Saved) {
  llvm::Module &M = CGM.getModule();
  for (const std::string &Name : Names) {
    llvm::Function *F = M.getFunction(Name);
    if (!F || F->isDeclaration())
      continue;
    llvm::Function *Ext = llvm::Function::Create(
        F->getFunctionType(), F->getLinkage(), F->getAddressSpace(),
        Name + ".contract.external", &M);
    Ext->copyAttributesFrom(F);
    Ext->copyMetadata(F, 0);
    Ext->splice(Ext->end(), F);
    for (auto [A, B] : llvm::zip(F->args(), Ext->args())) {
      B.takeName(&A);
      A.replaceAllUsesWith(&B);
    }
    Ext->addFnAttr(cs::ExternalAttr);
    F->setPersonalityFn(nullptr);
    F->clearMetadata();
    F->setLinkage(llvm::GlobalValue::ExternalLinkage);
    if (llvm::Function *Decl = Saved.lookup(Name)) {
      F->copyAttributesFrom(Decl);
      F->copyMetadata(Decl, 0);
      F->setLinkage(Decl->getLinkage());
    }
  }
  for (auto &Entry : Saved)
    Entry.second->eraseFromParent();
  Saved.clear();
}

void CodeGenModule::emitContractProofShadows() {
  // D4405: an -O0 module proving at a proof level above 0 says what the back
  // end strips from its real code, made for the proofs only.
  llvm::NamedMDNode *O0List = nullptr;
  if (ContractProofsAtO0) {
    O0List = getModule().getOrInsertNamedMetadata(cs::O0MD);
    unsigned Flags = cs::O0StripRange | (ContractProofTBAA ? cs::O0StripTBAA : 0);
    O0List->addOperand(llvm::MDNode::get(
        getLLVMContext(),
        llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(getLLVMContext()), Flags))));
  }
  if (!ContractShadowData)
    return;
  ContractShadowModuleState &St = *ContractShadowData;
  bool Shadows = !St.Pending.empty();
  // What is emitted for the proofs only leaves the real code and its debug
  // information as they are without the shadows.  The external vtables the
  // real code asks for are emitted speculatively (as available_externally)
  // before the shadows, while the inline virtual functions emitted are the
  // real code's: one a predicate alone calls would otherwise let a vtable
  // be emitted, and the real code be devirtualized through it (CLANG-914).
  // Those the shadows alone ask for are not emitted (cleared below).  And
  // everything the shadow phase emits is emitted without the module's debug
  // information: the types a predicate names (an explicit conversion's, a
  // constructor's class, a lambda's closure) and the vtables it loads would
  // be described in the real module (CLANG-915).
  std::unique_ptr<CGDebugInfo> SavedDebugInfo;
  if (Shadows) {
    EmitVTablesOpportunistically();
    SavedDebugInfo = std::move(DebugInfo);
  }
  // D4405: at -O0, once a shadow is made, the available_externally bodies the
  // real code did not emit are emitted for the proofs, each function's
  // attributes as a declaration kept.
  bool AtO0 = ContractProofsAtO0 && Shadows;
  llvm::StringMap<llvm::Function *> SavedDecls;
  if (AtO0) {
    ContractProofEmission = true;
    for (GlobalDecl GD : St.ExternalBodies) {
      StringRef Name = getMangledName(GD);
      llvm::Function *F = getModule().getFunction(Name);
      if (!F || !F->isDeclaration() || SavedDecls.count(Name))
        continue;
      llvm::Function *Decl = llvm::Function::Create(
          F->getFunctionType(), F->getLinkage(), F->getAddressSpace(), "",
          &getModule());
      Decl->copyAttributesFrom(F);
      Decl->copyMetadata(F, 0);
      SavedDecls[Name] = Decl;
      EmitGlobalDefinition(GD);
    }
  }
  // A shadow (or a premise copy) can make code generation emit more
  // functions (those only a predicate uses), which may defer evaluations of
  // their own.  The premise copies (D4405, ContractProvePremises.cpp) are
  // made only if a shadow is.
  auto CopiesLeft = [&] {
    return !St.Pending.empty() &&
           St.NextPremiseCopy < St.PremiseCopies.size();
  };
  while (St.NextPending < St.Pending.size() || CopiesLeft()) {
    while (St.NextPending < St.Pending.size()) {
      auto [GD, Name, Pair] = St.Pending[St.NextPending++];
      emitContractProofShadow(GD, Name, Pair);
    }
    while (CopiesLeft()) {
      auto [GD, Name] = St.PremiseCopies[St.NextPremiseCopy++];
      emitContractPremiseCopy(GD, Name);
    }
    EmitDeferred();
  }
  if (AtO0) {
    listContractO0Attributes(*this, O0List);
    moveContractExternalBodies(*this, St.ExternalNames, SavedDecls);
    ContractProofEmission = false;
  }
  if (Shadows) {
    DebugInfo = std::move(SavedDebugInfo);
    OpportunisticVTables.clear();
  }
  // D4405: the optsize and minsize the -O level implies are not the proof
  // level's; list the functions whose own attribute asks for one
  // (SetLLVMFunctionAttributesForDefinition: cold, minsize).
  if (St.Pending.empty() || !getCodeGenOpts().OptimizeSize)
    return;
  llvm::NamedMDNode *List = getModule().getOrInsertNamedMetadata(cs::SizeMD);
  llvm::LLVMContext &Ctx = getLLVMContext();
  for (llvm::Function &F : getModule()) {
    GlobalDecl GD;
    if (F.isDeclaration() || F.hasFnAttribute(cs::ShadowAttr) ||
        F.hasFnAttribute(cs::PremiseCopyAttr) ||
        !lookupRepresentativeDecl(F.getName(), GD))
      continue;
    const Decl *D = GD.getDecl();
    if (!D || D->hasAttr<OptimizeNoneAttr>())
      continue;
    bool OptSize = D->hasAttr<ColdAttr>(), MinSize = D->hasAttr<MinSizeAttr>();
    if (!OptSize && !MinSize)
      continue;
    llvm::Metadata *Ops[] = {
        llvm::ValueAsMetadata::get(&F),
        llvm::ConstantAsMetadata::get(llvm::ConstantInt::getBool(Ctx, OptSize)),
        llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::getBool(Ctx, MinSize))};
    List->addOperand(llvm::MDNode::get(Ctx, Ops));
  }
}

void CodeGenModule::reportContractProofsNow() {
  if (!ContractShadowData)
    return;
  // Each as decided, a deferred one unproven, combined with those nested in
  // it (DECISIONS.md E169).
  std::vector<ContractObligation> &Obs = ContractShadowData->Obligations;
  SmallVector<int, 16> Parent;
  SmallVector<bool, 16> Prove;
  SmallVector<ContractProofResult, 16> Raw;
  for (const ContractObligation &O : Obs) {
    Parent.push_back(O.Parent);
    Prove.push_back(O.Flags & cs::Prove);
    Raw.push_back(
        O.Decision == cs::TakenAlwaysFalse ? ContractProofResult::AlwaysFalse
        : O.Decision == cs::TakenSound     ? ContractProofResult::Sound
                                           : ContractProofResult::Unproven);
  }
  SmallVector<ContractProofResult, 16> Effective;
  SmallVector<SmallVector<unsigned, 2>, 16> Causes;
  combineNestedContractResults(Parent, Prove, Raw, Effective, Causes);
  // A nested diagnose evaluation's diagnostic is not issued again where the
  // evaluation at the same call in the check of the same place issues it
  // (DECISIONS.md E169 (9), (11)).
  std::set<std::tuple<const ContractStmt *, const Expr *, bool, unsigned>>
      Reported;
  for (size_t Id = 0; Id != Obs.size(); ++Id)
    if (Obs[Id].Parent < 0 && Obs[Id].CallSite &&
        !isContractProofProven(Effective[Id]))
      Reported.insert({Obs[Id].Contract, Obs[Id].CallSite,
                       Effective[Id] == ContractProofResult::AlwaysFalse,
                       Obs[Id].Group});
  for (size_t Id = 0; Id != Obs.size(); ++Id) {
    const ContractObligation &O = Obs[Id];
    // A nested prove evaluation is reported only through those it is nested
    // in.
    if ((O.Parent >= 0 && (O.Flags & cs::Prove)) ||
        isContractProofProven(Effective[Id]))
      continue;
    if (O.Parent >= 0 &&
        !Reported
             .insert({O.Contract, O.CallSite,
                      Effective[Id] == ContractProofResult::AlwaysFalse,
                      O.Group})
             .second)
      continue;
    SourceLocation Loc;
    SourceRange Range, PredRange;
    getObligationLocations(O, Loc, Range, PredRange);
    SmallVector<std::pair<SourceRange, bool>, 2> Notes;
    for (unsigned C : Causes[Id]) {
      std::pair<SourceRange, bool> Note{
          Obs[C].Contract->getCond()->getSourceRange(),
          Raw[C] == ContractProofResult::AlwaysFalse};
      if (!llvm::is_contained(Notes, Note))
        Notes.push_back(Note);
    }
    reportContractObligation(
        getDiags(), O.Flags, Loc, Range, PredRange, Effective[Id],
        &getContext(), O.DiagContext,
        O.Override ? StringRef(O.Override->BaseName) : StringRef(), Notes);
  }
  Obs.clear();
}

/// The notes of the nested prove obligations not proven (DECISIONS.md E169).
static void
noteNestedObligations(DiagnosticsEngine &Diags,
                      ArrayRef<std::pair<SourceRange, bool>> Nested) {
  for (auto [Range, AlwaysFalse] : Nested)
    Diags.Report(Range.getBegin(), diag::note_contract_nested_unproven)
        << AlwaysFalse << Range;
}

void CodeGen::reportContractObligation(
    DiagnosticsEngine &Diags, unsigned Flags, SourceLocation Loc,
    SourceRange Range, SourceRange PredRange, ContractProofResult R,
    const ASTContext *Ctx, unsigned DiagContext, StringRef OverrideName,
    ArrayRef<std::pair<SourceRange, bool>> Nested) {
  bool AlwaysFalse = R == ContractProofResult::AlwaysFalse;
  unsigned ID = (Flags & cs::Prove)
                    ? (AlwaysFalse ? diag::err_contract_always_false
                                   : diag::err_contract_unproven)
                    : (AlwaysFalse ? diag::warn_contract_always_false
                                   : diag::warn_contract_unproven);
  // Shown in a system header when the assertion is not in one
  // (notadragon_wg21 DECISIONS.md E186 (a); see CGContracts.cpp).
  bool UserAssertion =
      Diags.hasSourceManager() && PredRange.getBegin().isValid() &&
      !Diags.getSourceManager().isInSystemHeader(
          Diags.getSourceManager().getExpansionLoc(PredRange.getBegin()));
  ForceSystemWarningsRAII ShowForUserAssertion(Diags, UserAssertion);
  // D4405: an override pair's is at D::f's definition, naming B::f.
  bool Override = Flags & cs::Override;
  if (!(Flags & cs::CallerSide)) {
    Diags.Report(Loc, ID) << PredRange << Override << OverrideName;
    if (Ctx)
      Ctx->emitContractDiagnosticContext(Diags, DiagContext);
    noteNestedObligations(Diags, Nested);
    return;
  }
  Diags.Report(Loc, ID) << Range << Override << OverrideName;
  if (Ctx)
    Ctx->emitContractDiagnosticContext(Diags, DiagContext);
  // The notes follow the diagnostic, so they are dropped with a silenced
  // warning.
  Diags.Report(PredRange.getBegin(), diag::note_contract_unproven_predicate)
      << PredRange;
  noteNestedObligations(Diags, Nested);
}
