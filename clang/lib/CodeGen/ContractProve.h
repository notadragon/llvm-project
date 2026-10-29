//===--- ContractProve.h - P4405 proving strategies -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Proving strategies for the P4405 prove and diagnose evaluation semantics.
//
// An evaluation with prove or diagnose is diagnosed -- an error for prove, a
// -Wcontract-unproven warning for diagnose -- unless an enabled strategy
// proves its predicate.  The one emission point,
// CodeGenModule::diagnoseUnprovenContract, describes the evaluation in a
// ContractProofQuery and asks proveContract, which asks each enabled strategy
// in the registry (ContractProve.cpp) in turn.  Each strategy lives in a file
// of its own (ContractProveFold.cpp) behind ContractProofStrategy; nothing
// else in code generation knows which strategies exist.
//
// A deferring strategy (one that cannot answer at the emission point, such as
// the proof shadow, ContractProveShadow.cpp, decided by the back end) answers
// Deferred, which proveContract returns when no strategy proved the
// evaluation and a deferring one accepted it.  The emission point then asks
// placeDeferredContractProof to place that strategy's marker, through the
// CodeGenFunction in the query, instead of diagnosing; every later emission
// of the same evaluation (a constructor's other variant, a proof shadow) is
// placed the same way.  A strategy may also take over a diagnostic decided
// at once (takeContractDiagnostic), to issue it in order with the deferred
// ones.  D4405's override shadows (ContractProveOverride.cpp) are a third,
// deferring strategy, for the obligations of a virtual function made for a
// call through a function it overrides (ContractOverridePair).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVE_H
#define LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVE_H

#include "clang/Basic/ContractOptions.h"
#include "llvm/ADT/SmallVector.h"
#include <string>

namespace clang {
class ASTContext;
class ContractStmt;
class CXXBaseSpecifier;
class CXXMethodDecl;
class Expr;
class FunctionDecl;
class LangOptions;

namespace CodeGen {
class CodeGenFunction;

/// What a strategy concluded about one evaluation.
enum class ContractProofResult {
  /// Not proven: the evaluation is diagnosed, "cannot be proven".
  Unproven,
  /// Proven, relying on nothing that is not guaranteed to hold.
  Sound,
  /// Proven, but relying on an earlier assertion that is not guaranteed to
  /// hold (one evaluated with ignore, observe or noexcept_observe, or proven
  /// only conditionally).  Only a strategy whose MayDependOnAssumptions is
  /// set may answer this.
  Conditional,
  /// Not proven, and shown to be false whenever it is evaluated: diagnosed
  /// as "contract predicate is always false".  A refinement of Unproven.
  AlwaysFalse,
  /// Not decided here: a deferring strategy will decide, and diagnose, the
  /// evaluation later.  Only a strategy with PlaceDeferred may answer this.
  Deferred,
};

/// Whether \p R suppresses the diagnostic.
inline bool isContractProofProven(ContractProofResult R) {
  return R == ContractProofResult::Sound ||
         R == ContractProofResult::Conditional;
}

/// D4405, "Virtual Functions": an override pair, a defined virtual function
/// D::f and one function B::f it overrides, directly or indirectly (one per
/// B subobject when B::f's assertions name `this', DECISIONS.md E154).  Its
/// obligations -- D::f's preconditions and postconditions and B::f's
/// postconditions for a call through B::f (without override shadows, only
/// D::f's preconditions and B::f's postconditions) -- are evaluations of
/// D::f's or B::f's assertions made for that pair, at D::f's definition.
struct ContractOverridePair {
  /// D::f, the definition.
  const CXXMethodDecl *Derived = nullptr;
  /// B::f, and its declaration with the contract assertions (null if it has
  /// none: its pair has no premises, DECISIONS.md E156).
  const CXXMethodDecl *Base = nullptr;
  const FunctionDecl *BaseWithContracts = nullptr;
  /// The pair's evaluations' identity (with the assertion) and location:
  /// an expression standing for D::f's definition.
  const Expr *Site = nullptr;
  /// The derived-to-base path from D's class to the B subobject, which
  /// `this' takes.
  llvm::SmallVector<const CXXBaseSpecifier *, 4> Path;
  /// B::f's name, as a diagnostic names it.
  std::string BaseName;
  /// Its number in the module, from 1.
  unsigned Index = 0;
  /// Whether its obligations are decided in an override shadow of its own
  /// (-fcontracts-prove-override, the module optimized, D::f shadowable).
  bool Shadows = false;
};

/// One evaluation with prove or diagnose, as the emission point sees it.
struct ContractProofQuery {
  ASTContext &Ctx;
  /// The contract assertion.
  const ContractStmt &Contract;
  /// Prove or diagnose.
  ContractEvaluationSemantic Semantic;
  /// The predicate, as the check would evaluate it (instantiated, converted
  /// to bool).  A strategy must never evaluate it at run time or modify it.
  const Expr *Predicate;
  /// The call, for a caller-side evaluation; null for a callee-side one.
  const Expr *CallSite = nullptr;
  /// For a caller-side evaluation of a precondition, the function called,
  /// whose parameters the predicate names: a strategy may use the call's
  /// arguments for them.  Null otherwise.
  const FunctionDecl *Callee = nullptr;
  /// The function being emitted, in which a deferring strategy places its
  /// marker.  Null if there is none.
  CodeGenFunction *CGF = nullptr;
  /// D4405: for an obligation of an override pair, the pair (CallSite is its
  /// Site); null otherwise.
  const ContractOverridePair *Override = nullptr;
  /// With Override: decided in the pair's override shadow, with B::f's
  /// preconditions given (ContractProveOverrideStrategy); otherwise an
  /// obligation of D::f itself, with nothing given (DECISIONS.md E157).
  bool OverrideShadow = false;
};

/// A proving strategy.  A plain aggregate, so that the registry is
/// constant-initialized.
struct ContractProofStrategy {
  /// For dumps and debugging.
  const char *Name;
  /// Whether a proof may rely on earlier assertions (and so be Conditional).
  bool MayDependOnAssumptions;
  /// Whether the strategy is enabled (its -fcontracts-prove-* flag).
  bool (*IsEnabled)(const LangOptions &LangOpts);
  /// Try to prove \p Q's predicate.
  ContractProofResult (*Prove)(const ContractProofQuery &Q);
  /// For a deferring strategy, else null: place what the strategy emits for
  /// \p Q's evaluation in \p Q.CGF, if the evaluation is deferred to it --
  /// or, with \p Defer, defer it now (after Prove answered Deferred) -- and
  /// return whether it did.
  bool (*PlaceDeferred)(const ContractProofQuery &Q, bool Defer) = nullptr;
  /// Optional: take over the diagnostic of \p Q's evaluation, decided \p R
  /// (Unproven or AlwaysFalse) at once, to issue it later; return whether it
  /// did.
  bool (*TakeDiagnostic)(const ContractProofQuery &Q,
                         ContractProofResult R) = nullptr;
};

/// Ask each enabled strategy about \p Q: the first Sound or AlwaysFalse
/// answer wins, else Conditional if any strategy gave it, else Deferred if a
/// strategy deferred it, else Unproven.  With every strategy disabled,
/// Unproven.
ContractProofResult proveContract(const ContractProofQuery &Q,
                                  const LangOptions &LangOpts);

/// Have the enabled deferring strategy that holds \p Q's evaluation -- or,
/// with \p Defer, the first that accepts it -- place its marker; return
/// whether one did.
bool placeDeferredContractProof(const ContractProofQuery &Q,
                                const LangOptions &LangOpts, bool Defer);

/// Let an enabled strategy take over the diagnostic of \p Q's evaluation,
/// decided \p R at once; return whether one did.
bool takeContractDiagnostic(const ContractProofQuery &Q, ContractProofResult R,
                            const LangOptions &LangOpts);

} // namespace CodeGen
} // namespace clang

#endif // LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVE_H
