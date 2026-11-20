//===--- ContractFold.h - Folding contract predicates (P4405) ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// P4405: folding a contract predicate to prove it, as code generation's
// folding strategy (CodeGen/ContractProveFold.cpp) and Sema's check where a
// function's address is taken (SemaContract.cpp) both do.
//
// A predicate is folded with the constant evaluator in folding mode (not
// manifestly constant-evaluated), in a frame (ContractFoldFrame): for a
// precondition analyzed at a call, the call's arguments that fold to
// constants are its parameters' values and every other parameter is
// unknown, which keeps the proof sound; outside a call nothing is known.
//
// The analysis evaluation of a contract assertion also depends on the
// analysis evaluations of the preconditions of the calls its predicate
// makes, recursively (notadragon_wg21 DECISIONS.md E169): the nested
// evaluations.  forEachContractPredicateCall finds the calls a predicate
// evaluates, and decideNestedContractEvaluations decides the nested
// evaluations by folding, each in the frame of its call, whose arguments are
// folded in the frame of the predicate holding the call.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_CONTRACTFOLD_H
#define LLVM_CLANG_AST_CONTRACTFOLD_H

#include "clang/AST/APValue.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/ContractOptions.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>

namespace clang {
class ASTContext;
class ContractStmt;
class FunctionDecl;

/// The values a predicate is folded with: none outside a call; in a call,
/// those of the parameters whose arguments fold to constants, and `this' if
/// the object is a static one.
class ContractFoldFrame {
public:
  /// No frame: every parameter is unknown.
  ContractFoldFrame() = default;

  /// The frame of the call \p CallSite (a CallExpr or CXXConstructExpr) of
  /// \p Callee for \p Pred, a precondition of \p Callee: each by-value,
  /// non-volatile parameter of scalar type (integral, enumeration, floating,
  /// pointer, nullptr_t) whose argument folds, in \p Outer (no frame if
  /// null), to a constant-expression value without side effects or undefined
  /// behavior holds that value, and `this' designates the object when the
  /// implicit object argument folds (with no frame) to a static object.  The
  /// frame's function is the declaration whose parameters \p Pred names.  A
  /// lambda's call operator whose predicate names a capture gets no frame
  /// (captures are not modelled).
  ContractFoldFrame(const ASTContext &Ctx, const Expr *Pred,
                    const Expr *CallSite, const FunctionDecl *Callee,
                    const ContractFoldFrame *Outer);

  /// Fold \p E, a condition, to a boolean in this frame
  /// (Expr::EvaluateAsBooleanConditionInCall).
  bool evaluateAsBooleanCondition(const Expr *E, bool &Value,
                                  Expr::EvalStatus &Status,
                                  const ASTContext &Ctx) const;

  /// Fold \p E, evaluated in this frame, to a constant-expression value
  /// without side effects or undefined behavior.
  std::optional<APValue> foldValue(const Expr *E, const ASTContext &Ctx) const;

private:
  const FunctionDecl *Owner = nullptr;
  llvm::SmallVector<std::optional<APValue>, 4> Values;
  std::optional<APValue> This;

  /// The parameter values as Expr::EvaluateAsBooleanConditionInCall takes
  /// them.
  llvm::SmallVector<const APValue *, 4> paramValues() const;
};

/// What folding concluded about a predicate.
enum class ContractFoldResult {
  /// It did not fold, or folding met undefined behavior, or it may have side
  /// effects of its own that a proof would drop.
  Unknown,
  /// It folds to true.
  True,
  /// It folds to false.
  False,
};

/// Fold \p Pred, a contract predicate, in \p Frame.  The evaluator notes a
/// side effect for an operand it could not fold and went past
/// (`x > 0 || true`); only a predicate that may have side effects of its own
/// (`touch() || true`) is then Unknown.
ContractFoldResult foldContractPredicate(const ASTContext &Ctx,
                                         const Expr *Pred,
                                         const ContractFoldFrame &Frame);

/// Call \p F with each call \p Pred evaluates and the function it calls:
/// direct calls (member and operator calls included) and constructor calls,
/// in default arguments and default member initializers too, but not in an
/// unevaluated operand, a lambda's or block's body, a statement expression,
/// or the operand of `&&`, `||` or `?:` that folding in \p Frame shows is not
/// evaluated (every operand when \p Frame is null).
void forEachContractPredicateCall(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    llvm::function_ref<void(const Expr *Call, const FunctionDecl *Callee)> F);

/// The function whose contract assertions a call of \p FD evaluates (for an
/// inheriting constructor, the inherited one), with, in \p WithContracts, its
/// declaration holding them (null if none).
const FunctionDecl *
getContractEvaluatedCallee(const FunctionDecl *FD,
                           const FunctionDecl *&WithContracts);

/// The semantic of the analysis evaluation of \p Pre, a precondition of
/// \p WithContracts, at a call in C++: its own (resolved as for the callee's
/// definition) or what the caller-side resolution moved into it, whichever
/// is stronger.
ContractEvaluationSemantic
getContractCallAnalysisSemantic(const ASTContext &Ctx, const ContractStmt &Pre,
                                const FunctionDecl *WithContracts);

/// How deep nested evaluations are made (a precondition calling a function
/// whose precondition calls ...).
inline constexpr unsigned ContractNestedDepth = 8;

/// One nested evaluation (DECISIONS.md E169), decided by folding.
struct ContractNestedEvaluation {
  /// The precondition, the call it is evaluated at and the function called.
  const ContractStmt *Contract;
  const Expr *CallSite;
  const FunctionDecl *Callee;
  /// Prove or diagnose.
  ContractEvaluationSemantic Semantic;
  /// What folding concluded (Unknown when folding is off).
  ContractFoldResult Result;
  /// The index of the nested evaluation it is nested in, or -1 for one
  /// nested in the evaluation whose predicate the walk started at.
  int Parent;
};

/// Whether \p Contract may be nested in the evaluations of \p Chain (from the
/// outermost): it is not one of them (a precondition calling its own
/// function is not made again on the way) and the chain is shorter than
/// ContractNestedDepth.
bool canNestContractEvaluation(const ContractStmt &Contract,
                               llvm::ArrayRef<const ContractStmt *> Chain);

/// Whether \p Pred, the predicate of the evaluations \p Chain (from the
/// outermost; the last is \p Pred's own, or none for an outer evaluation
/// whose predicate is not a precondition), makes a nested evaluation: a call
/// it evaluates (folded in \p Frame) of a function with a precondition whose
/// analysis evaluation at the call is prove or diagnose.  Always false in C.
bool hasNestedContractEvaluations(const ASTContext &Ctx, const Expr *Pred,
                                  const ContractFoldFrame *Frame,
                                  llvm::ArrayRef<const ContractStmt *> Chain);

/// Decide by folding (only if \p Fold) the evaluations nested in an
/// evaluation of \p Pred in \p Frame, recursively through their own
/// predicates, appending each to \p Out with \p Parent (-1 for the outer one)
/// as its parent; \p Chain as for hasNestedContractEvaluations.  Each is
/// folded in the frame of its call, whose arguments are folded in \p Frame.
/// Nothing in C.
void decideNestedContractEvaluations(
    const ASTContext &Ctx, const Expr *Pred, const ContractFoldFrame *Frame,
    bool Fold, llvm::SmallVectorImpl<ContractNestedEvaluation> &Out, int Parent,
    llvm::ArrayRef<const ContractStmt *> Chain);

} // namespace clang

#endif // LLVM_CLANG_AST_CONTRACTFOLD_H
