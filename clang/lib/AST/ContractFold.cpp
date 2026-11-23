//===--- ContractFold.cpp - Folding contract predicates (P4405) -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// P4405: folding contract predicates, and deciding the evaluations nested in
// an analysis evaluation by folding (see ContractFold.h).
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ContractFold.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprConcepts.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/STLExtras.h"

using namespace clang;

//===----------------------------------------------------------------------===//
// Frames
//===----------------------------------------------------------------------===//

/// Whether a parameter of type \p T is substituted: by value (not a
/// reference) and of scalar type -- integral (bool and enumerations
/// included), floating, pointer or std::nullptr_t -- and not volatile.
static bool isSubstitutedParameterType(QualType T) {
  if (T.isVolatileQualified())
    return false;
  return T->isIntegralOrEnumerationType() || T->isRealFloatingType() ||
         T->isPointerType() || T->isNullPtrType();
}

/// Fold \p Arg, an object pointer or glvalue, with no frame, to a constant:
/// no side effects, no undefined behavior, and a constant-expression value,
/// so it designates a static object.
static std::optional<APValue> foldObject(const Expr *Arg, const ASTContext &Ctx,
                                         bool AsLValue) {
  if (Arg->isValueDependent())
    return std::nullopt;
  Expr::EvalResult R;
  bool Folded =
      AsLValue ? Arg->EvaluateAsLValue(R, Ctx) : Arg->EvaluateAsRValue(R, Ctx);
  if (!Folded || R.HasSideEffects || R.HasUndefinedBehavior)
    return std::nullopt;
  return std::move(R.Val);
}

/// The function whose parameters \p Pred names, if they are all one
/// redeclaration's of \p Callee; null if it names none or several.
static const FunctionDecl *getParameterOwner(const Expr *Pred,
                                             const FunctionDecl *Callee) {
  const FunctionDecl *Owner = nullptr;
  const Decl *Canon = Callee->getCanonicalDecl();
  llvm::SmallVector<const Stmt *, 16> Work{Pred};
  while (!Work.empty()) {
    const Stmt *S = Work.pop_back_val();
    if (!S)
      continue;
    if (const auto *DRE = dyn_cast<DeclRefExpr>(S))
      if (const auto *PVD = dyn_cast<ParmVarDecl>(DRE->getDecl()))
        if (const auto *FD = dyn_cast<FunctionDecl>(PVD->getDeclContext()))
          if (FD->getCanonicalDecl() == Canon) {
            if (Owner && Owner != FD)
              return nullptr;
            Owner = FD;
          }
    for (const Stmt *Child : S->children())
      Work.push_back(Child);
  }
  return Owner;
}

/// The arguments of \p CallSite aligned with the callee's parameters, and its
/// implicit object argument, if any.
static void getCallArguments(const Expr *CallSite,
                             SmallVectorImpl<const Expr *> &Args,
                             const Expr *&Object) {
  Object = nullptr;
  if (const auto *CE = dyn_cast<CXXConstructExpr>(CallSite)) {
    Args.append(CE->arg_begin(), CE->arg_end());
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

ContractFoldFrame::ContractFoldFrame(const ASTContext &Ctx, const Expr *Pred,
                                     const Expr *CallSite,
                                     const FunctionDecl *Callee,
                                     const ContractFoldFrame *Outer) {
  if (!CallSite || !Callee)
    return;
  const FunctionDecl *O = getParameterOwner(Pred, Callee);
  // A lambda's captures are not modelled by a frame made up here; a
  // predicate naming one is folded with every parameter unknown.
  if (!O || (isLambdaCallOperator(O) && namesLambdaCapture(Pred)))
    return;
  Owner = O;

  SmallVector<const Expr *, 8> Args;
  const Expr *Object;
  getCallArguments(CallSite, Args, Object);

  ContractFoldFrame None;
  if (!Outer)
    Outer = &None;
  unsigned NumParams = Owner->getNumParams();
  Values.resize(NumParams);
  for (unsigned I = 0, N = std::min<size_t>(NumParams, Args.size()); I != N;
       ++I)
    if (isSubstitutedParameterType(Owner->getParamDecl(I)->getType()))
      Values[I] = Outer->foldValue(Args[I], Ctx);

  const auto *MD = dyn_cast<CXXMethodDecl>(Owner);
  if (Object && MD && MD->isImplicitObjectMemberFunction()) {
    bool IsPointer = Object->isPRValue() && Object->getType()->isPointerType();
    if (IsPointer || Object->isGLValue())
      This = foldObject(Object, Ctx, /*AsLValue=*/!IsPointer);
    if (This && (!This->isLValue() || This->isNullPointer()))
      This.reset();
  }
}

SmallVector<const APValue *, 4> ContractFoldFrame::paramValues() const {
  SmallVector<const APValue *, 4> P;
  for (const std::optional<APValue> &V : Values)
    P.push_back(V ? &*V : nullptr);
  return P;
}

bool ContractFoldFrame::evaluateAsBooleanCondition(
    const Expr *E, bool &Value, Expr::EvalStatus &Status,
    const ASTContext &Ctx) const {
  if (E->isValueDependent())
    return false;
  SmallVector<const APValue *, 4> P = paramValues();
  return E->EvaluateAsBooleanConditionInCall(Value, Status, Ctx, Owner, P,
                                             This ? &*This : nullptr);
}

std::optional<APValue>
ContractFoldFrame::foldValue(const Expr *E, const ASTContext &Ctx) const {
  if (E->isValueDependent())
    return std::nullopt;
  SmallVector<const APValue *, 4> P = paramValues();
  Expr::EvalResult R;
  if (!E->EvaluateAsRValueInCall(R, Ctx, Owner, P, This ? &*This : nullptr) ||
      R.HasSideEffects || R.HasUndefinedBehavior)
    return std::nullopt;
  return std::move(R.Val);
}

ContractFoldResult
clang::foldContractPredicate(const ASTContext &Ctx, const Expr *Pred,
                             const ContractFoldFrame &Frame) {
  if (Pred->isValueDependent())
    return ContractFoldResult::Unknown;
  bool Value;
  Expr::EvalStatus Status;
  if (!Frame.evaluateAsBooleanCondition(Pred, Value, Status, Ctx) ||
      Status.HasUndefinedBehavior)
    return ContractFoldResult::Unknown;
  if (Status.HasSideEffects && Pred->HasSideEffects(Ctx))
    return ContractFoldResult::Unknown;
  return Value ? ContractFoldResult::True : ContractFoldResult::False;
}

//===----------------------------------------------------------------------===//
// The calls a predicate evaluates
//===----------------------------------------------------------------------===//

namespace {
class PredicateCallWalker {
  const ASTContext &Ctx;
  const ContractFoldFrame *Frame;
  llvm::function_ref<void(const Expr *, const FunctionDecl *)> F;

  /// What folding \p E in the frame gives: 1 true, 0 false, -1 unknown.
  int fold(const Expr *E) const {
    if (!Frame || !E || E->isValueDependent())
      return -1;
    bool Value;
    Expr::EvalStatus Status;
    if (!Frame->evaluateAsBooleanCondition(E, Value, Status, Ctx) ||
        Status.HasUndefinedBehavior)
      return -1;
    return Value;
  }

public:
  PredicateCallWalker(
      const ASTContext &Ctx, const ContractFoldFrame *Frame,
      llvm::function_ref<void(const Expr *, const FunctionDecl *)> F)
      : Ctx(Ctx), Frame(Frame), F(F) {}

  void walk(const Stmt *S) {
    if (!S)
      return;
    // Unevaluated operands, and code that is not evaluated where it appears.
    if (isa<UnaryExprOrTypeTraitExpr, CXXNoexceptExpr, RequiresExpr,
            ConceptSpecializationExpr, LambdaExpr, BlockExpr, StmtExpr,
            OpaqueValueExpr>(S))
      return;
    if (const auto *TE = dyn_cast<CXXTypeidExpr>(S)) {
      if (TE->isPotentiallyEvaluated())
        for (const Stmt *Child : TE->children())
          walk(Child);
      return;
    }
    if (const auto *GS = dyn_cast<GenericSelectionExpr>(S)) {
      if (!GS->isResultDependent())
        walk(GS->getResultExpr());
      return;
    }
    if (const auto *CE = dyn_cast<ChooseExpr>(S)) {
      if (!CE->isConditionDependent())
        walk(CE->getChosenSubExpr());
      return;
    }
    if (const auto *DA = dyn_cast<CXXDefaultArgExpr>(S)) {
      walk(DA->getExpr());
      return;
    }
    if (const auto *DI = dyn_cast<CXXDefaultInitExpr>(S)) {
      walk(DI->getExpr());
      return;
    }
    if (const auto *BO = dyn_cast<BinaryOperator>(S);
        BO && (BO->getOpcode() == BO_LAnd || BO->getOpcode() == BO_LOr)) {
      walk(BO->getLHS());
      int L = fold(BO->getLHS());
      if (L != (BO->getOpcode() == BO_LAnd ? 0 : 1))
        walk(BO->getRHS());
      return;
    }
    if (const auto *CO = dyn_cast<ConditionalOperator>(S)) {
      walk(CO->getCond());
      int C = fold(CO->getCond());
      if (C != 0)
        walk(CO->getTrueExpr());
      if (C != 1)
        walk(CO->getFalseExpr());
      return;
    }
    if (const auto *BCO = dyn_cast<BinaryConditionalOperator>(S)) {
      walk(BCO->getCommon());
      if (fold(BCO->getCommon()) != 1)
        walk(BCO->getFalseExpr());
      return;
    }
    if (const auto *Call = dyn_cast<CallExpr>(S)) {
      if (const FunctionDecl *Callee = Call->getDirectCallee())
        F(Call, Callee);
    } else if (const auto *Construct = dyn_cast<CXXConstructExpr>(S)) {
      F(Construct, Construct->getConstructor());
    }
    for (const Stmt *Child : S->children())
      walk(Child);
  }
};
} // namespace

void clang::forEachContractPredicateCall(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    llvm::function_ref<void(const Expr *, const FunctionDecl *)> F) {
  PredicateCallWalker(Ctx, Frame, F).walk(Pred);
}

//===----------------------------------------------------------------------===//
// Nested evaluations
//===----------------------------------------------------------------------===//

const FunctionDecl *
clang::getContractEvaluatedCallee(const FunctionDecl *FD,
                                  const FunctionDecl *&WithContracts) {
  WithContracts = nullptr;
  if (!FD)
    return nullptr;
  // A call of an inheriting constructor calls the inherited one, with the
  // same arguments, whose contracts are the ones evaluated.
  while (const auto *CD = dyn_cast<CXXConstructorDecl>(FD)) {
    InheritedConstructor Inh = CD->getInheritedConstructor();
    if (!Inh || Inh.getConstructor() == CD)
      break;
    FD = Inh.getConstructor();
  }
  for (const FunctionDecl *R : FD->redecls())
    if (R->hasContracts()) {
      WithContracts = R;
      break;
    }
  return FD;
}

ContractEvaluationSemantic
clang::getContractCallAnalysisSemantic(const ASTContext &Ctx,
                                       const ContractStmt &Pre,
                                       const FunctionDecl *WithContracts) {
  const DeclContext *DC = WithContracts->getDeclContext();
  return contractAnalysisStronger(Pre.ensureCallerAnalysisSemantic(Ctx, DC),
                                  Pre.ensureAnalysisSemantic(Ctx, DC));
}

bool clang::canNestContractEvaluation(
    const ContractStmt &Contract, llvm::ArrayRef<const ContractStmt *> Chain) {
  return Chain.size() <= ContractNestedDepth &&
         !llvm::is_contained(Chain, &Contract);
}

/// Call \p F with each nested evaluation of a call \p Pred evaluates:
/// the precondition, its semantic, the call and the function called.
static void forEachNestedEvaluation(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    llvm::ArrayRef<const ContractStmt *> Chain,
    llvm::function_ref<bool(const ContractStmt &, ContractEvaluationSemantic,
                            const Expr *, const FunctionDecl *)>
        F) {
  if (!Ctx.getLangOpts().CPlusPlus || !Ctx.getLangOpts().ContractsP4405 ||
      Pred->isValueDependent())
    return;
  bool Stop = false;
  forEachContractPredicateCall(
      Ctx, Pred, Frame, [&](const Expr *Call, const FunctionDecl *FD) {
        if (Stop)
          return;
        const FunctionDecl *WithContracts;
        const FunctionDecl *Callee =
            getContractEvaluatedCallee(FD, WithContracts);
        if (!WithContracts || isa<CXXDestructorDecl>(Callee))
          return;
        for (const ContractStmt *CS :
             WithContracts->getContracts()->contracts()) {
          if (CS->getContractKind() != ContractKind::Pre || CS->isInvalid() ||
              CS->getCond()->isValueDependent() ||
              !canNestContractEvaluation(*CS, Chain))
            continue;
          ContractEvaluationSemantic Sem =
              getContractCallAnalysisSemantic(Ctx, *CS, WithContracts);
          if (isProvingContractSemantic(Sem) && !F(*CS, Sem, Call, Callee)) {
            Stop = true;
            return;
          }
        }
      });
}

bool clang::hasNestedContractEvaluations(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    llvm::ArrayRef<const ContractStmt *> Chain) {
  bool Found = false;
  forEachNestedEvaluation(Ctx, Pred, Frame, Chain,
                          [&](const ContractStmt &, ContractEvaluationSemantic,
                              const Expr *, const FunctionDecl *) {
                            Found = true;
                            return false;
                          });
  return Found;
}

void clang::decideNestedContractEvaluations(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    bool Fold, llvm::SmallVectorImpl<ContractNestedEvaluation> &Out, int Parent,
    llvm::ArrayRef<const ContractStmt *> Chain) {
  if (!Fold)
    Frame = nullptr;
  forEachNestedEvaluation(
      Ctx, Pred, Frame, Chain,
      [&](const ContractStmt &CS, ContractEvaluationSemantic Sem,
          const Expr *Call, const FunctionDecl *Callee) {
        ContractFoldFrame Inner(Ctx, CS.getCond(), Call, Callee, Frame);
        ContractFoldResult R =
            Fold ? foldContractPredicate(Ctx, CS.getCond(), Inner)
                 : ContractFoldResult::Unknown;
        int Index = Out.size();
        Out.push_back({&CS, Call, Callee, Sem, R, Parent});
        llvm::SmallVector<const ContractStmt *, 8> Longer(Chain);
        Longer.push_back(&CS);
        decideNestedContractEvaluations(Ctx, CS.getCond(), &Inner, Fold, Out,
                                        Index, Longer);
        return true;
      });
}
