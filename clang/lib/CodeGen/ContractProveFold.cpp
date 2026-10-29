//===--- ContractProveFold.cpp - P4405 proving by folding -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The folding strategy, -fcontracts-prove-fold: a predicate that folds to
// true is proven, whether or not it is a constant expression, and one that
// folds to false is always false.  Folding is the constant evaluator's
// folding mode, not manifestly constant-evaluated (so is_constant_evaluated()
// is false, as at run time).  It assumes nothing but the language's rules,
// so its proofs are always sound.
//
// A caller-side evaluation of a precondition is folded as if in a call to
// the callee, with each by-value parameter of scalar type whose argument
// folds to a constant without side effects (or to the address of a static
// object) holding that value, and `this` designating the object when that
// is a static one.  Every other parameter is an unknown value, which keeps
// the proof sound.  The arguments are folded, never evaluated: the call
// evaluates them, once.
//
//===----------------------------------------------------------------------===//

#include "ContractProve.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>

using namespace clang;
using namespace CodeGen;

static bool isFoldEnabled(const LangOptions &LangOpts) {
  return LangOpts.ContractsProveFold;
}

/// Whether a parameter of type \p T is substituted: by value (not a
/// reference) and of scalar type -- integral (bool and enumerations
/// included), floating, pointer or std::nullptr_t -- and not volatile.
static bool isSubstitutedParameterType(QualType T) {
  if (T.isVolatileQualified())
    return false;
  return T->isIntegralOrEnumerationType() || T->isRealFloatingType() ||
         T->isPointerType() || T->isNullPtrType();
}

/// Fold \p Arg, an argument (or object pointer), to a constant: no side
/// effects, no undefined behavior, and a constant-expression value, so a
/// pointer designates a static object or function.
static std::optional<APValue> foldArgument(const Expr *Arg,
                                           const ASTContext &Ctx,
                                           bool AsLValue = false) {
  if (Arg->isValueDependent())
    return std::nullopt;
  Expr::EvalResult R;
  bool Folded = AsLValue ? Arg->EvaluateAsLValue(R, Ctx)
                         : Arg->EvaluateAsRValue(R, Ctx);
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

/// Fold \p Q's predicate; with \p Q's call's arguments substituted for the
/// parameters where it is a caller-side precondition.
static bool foldPredicate(const ContractProofQuery &Q, bool &Value,
                          Expr::EvalStatus &Status) {
  const FunctionDecl *Owner =
      Q.Callee && Q.CallSite ? getParameterOwner(Q.Predicate, Q.Callee)
                             : nullptr;
  // A lambda's captures are not modelled by a frame made up here; a
  // predicate naming one is folded with every parameter unknown.
  if (!Owner ||
      (isLambdaCallOperator(Owner) && namesLambdaCapture(Q.Predicate)))
    return Q.Predicate->EvaluateAsBooleanConditionInCall(Value, Status, Q.Ctx);

  SmallVector<const Expr *, 8> Args;
  const Expr *Object;
  getCallArguments(Q.CallSite, Args, Object);

  unsigned NumParams = Owner->getNumParams();
  SmallVector<std::optional<APValue>, 8> Values(NumParams);
  SmallVector<const APValue *, 8> ParamValues(NumParams, nullptr);
  for (unsigned I = 0, N = std::min<size_t>(NumParams, Args.size()); I != N;
       ++I) {
    if (!isSubstitutedParameterType(Owner->getParamDecl(I)->getType()))
      continue;
    Values[I] = foldArgument(Args[I], Q.Ctx);
    if (Values[I])
      ParamValues[I] = &*Values[I];
  }

  std::optional<APValue> ThisValue;
  const auto *MD = dyn_cast<CXXMethodDecl>(Owner);
  if (Object && MD && MD->isImplicitObjectMemberFunction()) {
    bool IsPointer = Object->isPRValue() && Object->getType()->isPointerType();
    if (IsPointer || Object->isGLValue())
      ThisValue = foldArgument(Object, Q.Ctx, /*AsLValue=*/!IsPointer);
    if (ThisValue && (!ThisValue->isLValue() || ThisValue->isNullPointer()))
      ThisValue.reset();
  }

  return Q.Predicate->EvaluateAsBooleanConditionInCall(
      Value, Status, Q.Ctx, Owner, ParamValues,
      ThisValue ? &*ThisValue : nullptr);
}

static ContractProofResult proveByFolding(const ContractProofQuery &Q) {
  if (Q.Predicate->isValueDependent())
    return ContractProofResult::Unproven;
  bool Value;
  Expr::EvalStatus Status;
  if (!foldPredicate(Q, Value, Status) || Status.HasUndefinedBehavior)
    return ContractProofResult::Unproven;
  // The evaluator notes a side effect for an operand it could not fold and
  // went past (`x > 0 || true`).  Only a predicate that may have side effects
  // of its own (`touch() || true`) is then unproven: its value is known, but
  // proving it would drop those effects.
  if (Status.HasSideEffects && Q.Predicate->HasSideEffects(Q.Ctx))
    return ContractProofResult::Unproven;
  return Value ? ContractProofResult::Sound : ContractProofResult::AlwaysFalse;
}

const ContractProofStrategy CodeGen::ContractProveFoldStrategy = {
    "fold", /*MayDependOnAssumptions=*/false, isFoldEnabled, proveByFolding};
