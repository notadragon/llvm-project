//===--- ContractProveShadow.h - P4405 proof shadows -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The proof shadow strategy, -fcontracts-prove-shadow (P4405 stage 3).
//
// An evaluation with prove or diagnose that folding does not settle is
// deferred: code generation emits, for each function holding one, a second
// copy of the function -- its proof shadow, internal and never emitted -- in
// which the evaluation is an obligation, `if (!pred)
// __clang_contract_unproven(id)` (a noreturn call), and earlier evaluations
// are assumptions.  The function's own code is the same as with ignore.
//
// Before the optimization pipeline runs, the back end
// (decideContractProofs, called from BackendUtil) moves the shadows, and the
// functions they use, into a separate module, optimizes that module with the
// pipeline the -O level selects, and decides each obligation: proven if no
// call to __clang_contract_unproven for it survives.  Then it diagnoses, in
// the order code generation met them, every evaluation not proven, and
// discards the proof module, so nothing of a shadow reaches the output, and
// the real module is optimized as it would have been without one.  Under
// -fcontracts-prove-assume a soundly proven evaluation becomes an
// assumption in the real function (decided before the real module is
// optimized, so it never meets an undecided one).
//
// A premise a terminating check makes is guaranteed only where this TU's
// code decides it (or its allowed set holds no continuing semantic): code
// generation lists the functions holding such checks, and the back end makes
// a shadow relying on another TU's possible definition conditional, or, for
// a proof it keeps as an assumption, makes the real code inline what the
// shadow's proof inlined.
//
// D4405's override shadows (-fcontracts-prove-override,
// ContractProveOverride.cpp) are proof shadows too: one more copy of a virtual
// function D::f per function B::f it overrides, which takes B::f's
// preconditions as given and holds the obligations of that override pair.
// An ordinary obligation met there is a premise, and the override shadow's
// proofs are only conditional unless every such premise is proven soundly;
// no override proof is ever an assumption in the real code.
//
// ContractProveShadow.cpp is the code generation side, ContractShadowDecide.cpp
// the back end side; this header is what they, ContractProveOverride.cpp,
// CodeGenModule, CodeGenFunction and BackendUtil share.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVESHADOW_H
#define LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVESHADOW_H

#include "Address.h"
#include "ContractProve.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include <deque>
#include <string>
#include <tuple>
#include <vector>

namespace llvm {
class Instruction;
class Value;
class Module;
enum class OptimizationLevel;
class PassBuilder;
class PipelineTuningOptions;
class TargetLibraryInfoImpl;
class TargetMachine;
namespace vfs {
class FileSystem;
} // namespace vfs
} // namespace llvm

namespace clang {
class DiagnosticsEngine;

namespace CodeGen {
class CodeGenFunction;
class CodeGenModule;

/// Names shared by code generation and the back end.
namespace contract_shadow {
/// Named metadata listing every evaluation decided in the back end, in the
/// order code generation met them (see ContractObligation).
inline constexpr llvm::StringLiteral ObligationsMD =
    "clang.contract.obligations";
/// Function attribute marking a proof shadow; "conditional" when the shadow
/// holds an assumption that is not guaranteed.
inline constexpr llvm::StringLiteral ShadowAttr = "clang.contract.proof";
/// D4405: function attribute marking a proof shadow as an override shadow,
/// whose value is its pair's number.
inline constexpr llvm::StringLiteral OverrideAttr = "clang.contract.override";
/// D4405: function metadata on an override shadow, {i32 id...}: the ordinary
/// obligations it takes as premises, whose results bound its proofs.
inline constexpr llvm::StringLiteral ReliedMD = "clang.contract.relied";
/// Instruction metadata, {i32 id}, on a call or a store emitted for the
/// predicate of obligation id.
inline constexpr llvm::StringLiteral PredicateMD = "clang.contract.pred";
/// Instruction metadata on the llvm.assume of an assumption made of a proven
/// evaluation (ContractAssumeCleanup removes it at the end of optimization).
inline constexpr llvm::StringLiteral AssumeMD = "clang.contract.assume";
/// Named metadata listing, once each, the functions whose code holds a
/// terminating check (enforce, noexcept_enforce, quick_enforce) of an
/// assertion whose allowed set admits a continuing semantic: premises only
/// this TU's copy of the function decides.
inline constexpr llvm::StringLiteral PremisesMD = "clang.contract.premises";
/// Named metadata listing, once each, the functions whose code holds a
/// contract evaluation that is not settled (CodeGenFunction::
/// noteContractEvaluation): another TU's copy may differ from this one in
/// what it does as well as in what it relies on.  A superset of PremisesMD.
inline constexpr llvm::StringLiteral RefinableMD = "clang.contract.refinable";
/// Instruction metadata on a call the back end made always_inline in the real
/// module because a sound proof relied on the shadow's inlining of it.
inline constexpr llvm::StringLiteral InlineMD = "clang.contract.inline";
/// void(i32): an obligation's failure in a shadow; noreturn.
inline constexpr llvm::StringLiteral UnprovenFn = "__clang_contract_unproven";
/// i1(i32): in a real function under -fcontracts-prove-assume, whether the
/// assumption made of an obligation is kept; replaced before optimization.
inline constexpr llvm::StringLiteral ProvenFn = "__clang_contract_proven";

/// Flags of an obligation record.
enum : unsigned {
  Prove = 1,           // prove (otherwise diagnose)
  CallerSide = 2,      // at a call (otherwise in the function's body)
  InLoop = 4,          // inside a loop or switch when emitted
  Unshadowed = 8,      // emitted somewhere no shadow covers: unproven
  Override = 16,       // an override pair's: "... for a call through 'B::f'"
  OverrideShadow = 32, // decided in its pair's override shadow
};

/// The decision recorded for an obligation by code generation.
enum : unsigned {
  Deferred = 0,         // decided by the shadows
  TakenUnproven = 1,    // decided unproven at once
  TakenAlwaysFalse = 2, // decided always false at once
};
} // namespace contract_shadow

/// One evaluation the back end decides or diagnoses.
struct ContractObligation {
  const ContractStmt *Contract;
  const Expr *CallSite;
  const FunctionDecl *Callee;
  ContractEvaluationSemantic Semantic;
  unsigned Flags = 0;
  unsigned Decision = contract_shadow::Deferred;
  /// Its diagnostic's template instantiation context
  /// (ASTContext::getContractDiagnosticContext), or 0.
  unsigned DiagContext = 0;
  /// D4405: the override pair it belongs to, or null.
  const ContractOverridePair *Override = nullptr;
};

/// The strategy's state for a module (CodeGenModule::getContractShadowState).
struct ContractShadowModuleState {
  /// Every obligation; the index is its id.
  std::vector<ContractObligation> Obligations;
  llvm::DenseMap<std::pair<const ContractStmt *, const Expr *>, unsigned>
      ObligationIds;
  /// Functions whose shadow is still to be emitted, by mangled name, with
  /// the override pair whose override shadow it is (0 for its proof shadow).
  std::vector<std::tuple<GlobalDecl, std::string, unsigned>> Pending;
  llvm::StringSet<> PendingNames;
  size_t NextPending = 0;
  /// Whether shadows are made in this module at all (isContractShadowActive).
  int Active = -1;
  /// Whether nothing in this module rules shadows out
  /// (canMakeContractShadows).
  int CanShadow = -1;
  /// D4405: the override pairs, numbered from 1, by (D::f, B::f) canonical
  /// declarations and B's subobject (DECISIONS.md E154: 0 unless B::f's
  /// contract assertions name `this'), and the numbers of each D::f's.
  std::deque<ContractOverridePair> OverridePairs;
  llvm::DenseMap<std::tuple<const Decl *, const Decl *, unsigned>, unsigned>
      OverridePairIds;
  llvm::DenseMap<const Decl *, llvm::SmallVector<unsigned, 2>> OverridePairsOf;
  /// The functions already listed in the premise list (PremisesMD).
  llvm::StringSet<> PremiseNames;
  /// The functions already listed in the refinable list (RefinableMD).
  llvm::StringSet<> RefinableNames;
};

/// D4405: what an override pair's B::f assertions see in a shadow of D::f:
/// `this' converted to the B subobject at entry and the parameters (by
/// index), D::f's own or, for a by-value one D::f may modify, a copy made at
/// entry (DECISIONS.md E153).
struct ContractOverrideSnapshot {
  llvm::Value *This = nullptr;
  llvm::SmallVector<Address, 4> Params;
};

/// The state of a function being emitted that the strategy needs: set while
/// a proof shadow is emitted, and while the predicate of an obligation is.
struct ContractShadowFunction {
  /// Emitting a proof shadow (otherwise a real function).
  bool InShadow = false;
  /// The obligation whose predicate is being emitted, or -1.
  int PredicateTag = -1;
  /// The shadow holds an assumption that is not guaranteed.
  bool UnguaranteedAssumption = false;
  /// D4405: in an override shadow, its pair; the ordinary obligations it
  /// takes as premises; and the assertions it holds as the pair's
  /// obligations, whose own evaluations after them assume nothing.
  const ContractOverridePair *Override = nullptr;
  llvm::SmallVector<unsigned, 4> Relied;
  llvm::SmallPtrSet<const ContractStmt *, 4> OverrideObligations;
  /// D4405: the override pairs' snapshots taken at entry.
  llvm::DenseMap<const ContractOverridePair *, ContractOverrideSnapshot>
      Snapshots;
};

/// Whether proof shadows of any kind can be made in \p CGM's module: it is
/// optimized and nothing rules them out (isContractShadowActive, without the
/// flag).
bool canMakeContractShadows(CodeGenModule &CGM);

/// Record a new obligation for \p Q's evaluation, decided \p Decision
/// (contract_shadow::Deferred, TakenUnproven or TakenAlwaysFalse), in the
/// module's list; return its id.
unsigned addContractObligation(CodeGenModule &CGM, const ContractProofQuery &Q,
                               unsigned Decision);

/// D4405: `this' converted to \p P's B subobject, in a function of D::f's.
llvm::Value *getContractOverrideThis(CodeGenFunction &CGF,
                                     const ContractOverridePair &P);

/// Whether no TU can continue past a violation of \p S: its allowed set
/// holds only terminating semantics and assume, so its evaluation is a
/// guaranteed premise wherever its code runs.
bool isContractPremiseSettled(const ContractStmt &S);

/// Diagnose obligation \p O, decided \p R (Unproven or AlwaysFalse), with the
/// stage 1 texts: at the assertion's keyword, the predicate highlighted, for
/// one in a function body; at the call, with a note at the predicate, for one
/// at a call -- or at D::f's definition, naming \p OverrideName, for an
/// override pair's (D4405).  In a template instantiation, followed by the
/// instantiation notes of \p DiagContext, held by \p Ctx (none without one).
void reportContractObligation(DiagnosticsEngine &Diags, unsigned Flags,
                              SourceLocation Loc, SourceRange Range,
                              SourceRange PredRange, ContractProofResult R,
                              const ASTContext *Ctx, unsigned DiagContext,
                              llvm::StringRef OverrideName = {});

/// BackendUtil: whether \p M holds anything decideContractProofs handles.
bool moduleHasContractProofs(const llvm::Module &M);

/// BackendUtil, before the optimization pipeline: decide the proof shadows of
/// \p M in a module of their own, optimized with \p TM, \p PTO, \p TLII at
/// \p Level; diagnose every recorded evaluation not proven; remove every
/// trace of the shadows from \p M and keep the assumptions made of sound
/// proofs if \p AllowAssumptions (delete them otherwise).  \p Verify checks
/// the proof module before optimizing it; \p FS is the file system the
/// pipeline may use; \p Ctx, the AST the module was generated from (null
/// for IR input), holds the diagnostics' instantiation notes.  Returns
/// whether an assumption was kept.
bool decideContractProofs(llvm::Module &M, llvm::TargetMachine *TM,
                          const llvm::PipelineTuningOptions &PTO,
                          const llvm::TargetLibraryInfoImpl &TLII,
                          llvm::OptimizationLevel Level, bool Verify,
                          bool AllowAssumptions,
                          llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS,
                          DiagnosticsEngine &Diags, const ASTContext *Ctx);

/// BackendUtil, when decideContractProofs kept an assumption: at the end of
/// the optimization pipeline, delete the assumptions made of proven
/// evaluations, the predicate code only they use and every function only
/// that code used -- what code generation would delete anyway, but for the
/// functions, which would otherwise be emitted.
void registerContractAssumeCleanup(llvm::PassBuilder &PB);

} // namespace CodeGen
} // namespace clang

#endif // LLVM_CLANG_LIB_CODEGEN_CONTRACTPROVESHADOW_H
