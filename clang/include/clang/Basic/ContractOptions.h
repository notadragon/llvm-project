//===- ContractOptions.h - C++ Contract Options -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// Defines common enums and types used by contracts and contract attributes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H
#define LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H

#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace llvm {
namespace vfs {
class FileSystem;
} // namespace vfs
} // namespace llvm

namespace clang {
using llvm::StringRef;
class ASTContext;
class DeclContext;
class DiagnosticsEngine;
class SourceManager;

enum class ContractGroupDiagnostic {
  // The remaining values map to %select values in diagnostics in both
  // DiagnosticSemaKinds.td and DiagnosticDriverKinds.td.
  InvalidFirstChar,
  InvalidChar,
  InvalidLastChar,
  EmptySubGroup,
  Empty,
  InvalidSemantic,
  // -fcontract-group-evaluation-semantic= only: a semantic with no group.
  MissingGroup,
};

/// The kind of contract, as classified internally by the implementation.
///
/// NOTE: This is deliberately distinct from ContractAssertionKind (the
/// ABI-visible std::contracts::assertion_kind) and is not a renumbering of
/// it: Implicit has no assertion_kind counterpart, and assertion_kind's
/// Manual/CAssert have no ContractKind counterpart.  Do not cast between
/// them.
enum class ContractKind {
  /// A function precondition
  Pre,

  /// A function post condition
  Post,

  /// contract_assert
  Assert,

  /// P3100 implicit contract assertion guarding core-language UB
  Implicit
};

/// Width of the packed ContractKind field in a serialized ContractStmt.
///
/// The reader and the writer must agree on this, and they are in different
/// files, so neither may spell it as a literal.
inline constexpr unsigned NumContractKindBits = 2;
static_assert(static_cast<unsigned>(ContractKind::Implicit) <
                  (1u << NumContractKindBits),
              "NumContractKindBits is too narrow for ContractKind");

/// std::contracts::assertion_kind, as a violation reports it.
enum class ContractAssertionKind {
  Pre = 1,
  Post = 2,
  Assert = 3,
  Manual = 4,
  CAssert = 5,
  PostCapture = 6,
  Implicit = 7
};

/// The assertion kind a violation of a contract of kind \p Kind reports;
/// \p IsPostCapture selects the initialization of a postcondition's captures.
inline ContractAssertionKind getContractAssertionKind(ContractKind Kind,
                                                      bool IsPostCapture) {
  if (IsPostCapture)
    return ContractAssertionKind::PostCapture;
  switch (Kind) {
  case ContractKind::Pre:
    return ContractAssertionKind::Pre;
  case ContractKind::Post:
    return ContractAssertionKind::Post;
  case ContractKind::Assert:
    return ContractAssertionKind::Assert;
  case ContractKind::Implicit:
    return ContractAssertionKind::Implicit;
  }
  llvm_unreachable("unknown ContractKind");
}

/// Contract evaluation mode. Determines whether to check contracts, and
/// whether contract failures cause compile errors.
///
/// Values match std::contracts::evaluation_semantic per
/// [support.contract.enum].
enum class ContractEvaluationSemantic {
  Ignore = 1,
  Observe = 2,
  Enforce = 3,
  QuickEnforce = 4,
  Assume = 5,          // P3100 "assume" evaluation semantic
  NoexceptObserve = 6, // P4298
  NoexceptEnforce = 7, // P4298
  // P4405: no run-time effect; an evaluation is diagnosed at compile time
  // unless its predicate is proven -- an error for Prove, a warning
  // (-Wcontract-unproven) for Diagnose.
  Prove = 8,
  Diagnose = 9,
};

/// The first and last ContractEvaluationSemantic values; every value between
/// them is one.
constexpr unsigned FirstContractEvaluationSemantic =
    static_cast<unsigned>(ContractEvaluationSemantic::Ignore);
constexpr unsigned LastContractEvaluationSemantic =
    static_cast<unsigned>(ContractEvaluationSemantic::Diagnose);

/// Not a ContractEvaluationSemantic: what the best fit (clampToAllowed)
/// yields when there is no valid evaluation semantic.
inline constexpr ContractEvaluationSemantic InvalidContractEvaluationSemantic =
    static_cast<ContractEvaluationSemantic>(0);

/// Whether \p V is the value of a ContractEvaluationSemantic.
constexpr bool isContractEvaluationSemanticValue(int64_t V) {
  return V >= FirstContractEvaluationSemantic &&
         V <= LastContractEvaluationSemantic;
}

/// Bitmask (bit N == ContractEvaluationSemantic value N) of the four standard
/// evaluation semantics.  This is the default "all allowed" set for a
/// contract's allowed-semantics mask.  QuickEnforce is deliberately part of
/// it: omitting a semantic from the mask sends a contract that resolves to it
/// down the fallback chain (see clampToAllowed), so the default set must name
/// every standard semantic explicitly.
inline constexpr unsigned AllContractSemanticsMask =
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Observe)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Enforce)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::QuickEnforce));

/// Every valid evaluation semantic, including P3100 "assume".  This is the
/// flag-independent "no restriction" set that a label's allowed_semantics
/// facet narrows; the -fcontracts-allow-assume gate is applied separately.
inline constexpr unsigned AllContractSemanticsMaskWithAssume =
    AllContractSemanticsMask |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Assume));

/// The P4405 prove and diagnose semantics.
inline constexpr unsigned ProvingContractSemanticsMask =
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Prove)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Diagnose));

/// Every valid evaluation semantic, including "assume", the P4298
/// noexcept-terminating variants and the P4405 prove and diagnose.  This is
/// the flag-independent "no restriction" set that a label's
/// allowed_semantics facet narrows; the -fcontracts-allow-assume /
/// -fcontracts-p4298 / -fcontracts-p4405 gates are applied separately.
inline constexpr unsigned AllContractSemanticsMaskWithExtensions =
    AllContractSemanticsMaskWithAssume |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptEnforce)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::NoexceptObserve)) |
    ProvingContractSemanticsMask;

/// Width of a ContractStmt's allowed-semantics mask, which it holds in a
/// uint16_t member: bit N for every semantic value N up to
/// LastContractEvaluationSemantic.
inline constexpr unsigned NumContractAllowedMaskBits = 16;
static_assert(AllContractSemanticsMaskWithExtensions <
                  (1u << NumContractAllowedMaskBits),
              "NumContractAllowedMaskBits is too narrow for the semantics "
              "mask");

/// The gated base set: the four standard semantics, plus "assume" only when
/// -fcontracts-allow-assume is in effect, the P4298 noexcept variants only
/// under -fcontracts-p4298, and the P4405 prove and diagnose only under
/// -fcontracts-p4405.  The (flag-independent) label restriction is
/// intersected with this at query construction, so a gated semantic can
/// never be present when its flag is off -- a label cannot re-add it.
inline constexpr unsigned gatedContractSemanticsMask(bool AllowAssume,
                                                     bool EnableP4298,
                                                     bool EnableP4405) {
  unsigned Mask = AllowAssume ? AllContractSemanticsMaskWithAssume
                              : AllContractSemanticsMask;
  if (EnableP4298)
    Mask |= (1u << static_cast<unsigned>(
                 ContractEvaluationSemantic::NoexceptEnforce)) |
            (1u << static_cast<unsigned>(
                 ContractEvaluationSemantic::NoexceptObserve));
  if (EnableP4405)
    Mask |= ProvingContractSemanticsMask;
  return Mask;
}

/// True if \p S is one of the P4405 semantics, prove and diagnose: the
/// contract stays active up to the point where its check would be emitted,
/// but no check is emitted and the predicate is never evaluated; instead an
/// evaluation that cannot be proven is diagnosed there.  Everywhere nothing
/// would be diagnosed -- the initialization of postcondition captures, the
/// value a dynamic selector returns -- they behave exactly as ignore.  D4405:
/// they apply only to the analysis evaluation; constant evaluation, which is
/// never analyzed, uses enforce and observe for them
/// (contractConstantEvaluationSemantic).
constexpr bool isProvingContractSemantic(ContractEvaluationSemantic S) {
  return S == ContractEvaluationSemantic::Prove ||
         S == ContractEvaluationSemantic::Diagnose;
}

/// D4405: the analysis evaluation of a contract assertion -- its first, the
/// only one prove or diagnose is ever applied to -- has one of ignore,
/// diagnose and prove, ordered by strength: ignore < diagnose < prove.
/// Return the stronger of \p A and \p B, each one of the three.
constexpr ContractEvaluationSemantic
contractAnalysisStronger(ContractEvaluationSemantic A,
                         ContractEvaluationSemantic B) {
  auto Rank = [](ContractEvaluationSemantic S) {
    return S == ContractEvaluationSemantic::Prove      ? 2
           : S == ContractEvaluationSemantic::Diagnose ? 1
                                                       : 0;
  };
  return Rank(B) > Rank(A) ? B : A;
}

/// D4405: the allowed set A of a contract assertion (\p Mask, the label's set
/// intersected with the semantics the flags enable) splits into its runtime
/// part R, the semantics other than prove and diagnose, and its analysis part
/// N, those two.  The allowed set of the analysis evaluation is N, plus
/// ignore when R is not empty (a runtime evaluation then has an allowed
/// semantic).
constexpr unsigned contractAnalysisAllowedMask(unsigned Mask) {
  unsigned N = Mask & ProvingContractSemanticsMask;
  if (Mask & ~ProvingContractSemanticsMask)
    N |= 1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore);
  return N;
}

/// D4405: the allowed set of a runtime evaluation of a contract assertion
/// whose allowed set is \p Mask.  A caller-side evaluation may always be
/// ignore: the callee-side evaluation is made with an allowed semantic.  A
/// callee-side one has R, or {ignore} when R is empty, since the analysis
/// evaluation must then be prove or diagnose in every translation unit --
/// decided by the labels alone, never by the configuration.  Zero when both R
/// and N are empty, which is ill-formed.  (A constant evaluation is adjusted
/// to \p Mask itself: contractConstantEvaluationSemantic.)
constexpr unsigned contractRuntimeAllowedMask(unsigned Mask, bool CallerSide) {
  unsigned R = Mask & ~ProvingContractSemanticsMask;
  if (CallerSide || (!R && (Mask & ProvingContractSemanticsMask)))
    R |= 1u << static_cast<unsigned>(ContractEvaluationSemantic::Ignore);
  return R;
}

/// D4405 (notadragon_wg21 DECISIONS.md E142): constant evaluation is never
/// analyzed, and a prove or diagnose it reaches -- from the configuration, a
/// flag, or a label's allowed set or compute_semantic -- is not ignore: the
/// one evaluation being made must satisfy a prove, as under enforce (a
/// violation makes the expression not a constant expression), and a diagnose
/// is observe (a violation is a warning).  Return the semantic a constant
/// evaluation uses for \p S.
constexpr ContractEvaluationSemantic
contractConstantEvaluationSemantic(ContractEvaluationSemantic S) {
  if (S == ContractEvaluationSemantic::Prove)
    return ContractEvaluationSemantic::Enforce;
  if (S == ContractEvaluationSemantic::Diagnose)
    return ContractEvaluationSemantic::Observe;
  return S;
}

/// Parse a contract evaluation semantic name (e.g. "observe",
/// "noexcept_enforce") to its enum value.  Returns false if NAME does not
/// name a known semantic.  Single source of truth for semantic-name spelling,
/// shared by -fcontract-group-evaluation-semantic= parsing and the P3100
/// -fsanitize-semantic= driver parsing.
inline bool contractSemanticFromName(StringRef Name,
                                     ContractEvaluationSemantic &Out) {
  if (Name == "ignore")
    Out = ContractEvaluationSemantic::Ignore;
  else if (Name == "observe")
    Out = ContractEvaluationSemantic::Observe;
  else if (Name == "enforce")
    Out = ContractEvaluationSemantic::Enforce;
  else if (Name == "quick_enforce")
    Out = ContractEvaluationSemantic::QuickEnforce;
  else if (Name == "assume")
    Out = ContractEvaluationSemantic::Assume;
  else if (Name == "noexcept_enforce")
    Out = ContractEvaluationSemantic::NoexceptEnforce;
  else if (Name == "noexcept_observe")
    Out = ContractEvaluationSemantic::NoexceptObserve;
  else if (Name == "prove")
    Out = ContractEvaluationSemantic::Prove;
  else if (Name == "diagnose")
    Out = ContractEvaluationSemantic::Diagnose;
  else
    return false;
  return true;
}

/// The canonical spelling for a contract evaluation semantic value; the
/// inverse of contractSemanticFromName.  Returns "invalid" for an
/// out-of-range value.
inline StringRef contractSemanticName(ContractEvaluationSemantic Semantic) {
  switch (Semantic) {
  case ContractEvaluationSemantic::Ignore:
    return "ignore";
  case ContractEvaluationSemantic::Observe:
    return "observe";
  case ContractEvaluationSemantic::Enforce:
    return "enforce";
  case ContractEvaluationSemantic::QuickEnforce:
    return "quick_enforce";
  case ContractEvaluationSemantic::Assume:
    return "assume";
  case ContractEvaluationSemantic::NoexceptEnforce:
    return "noexcept_enforce";
  case ContractEvaluationSemantic::NoexceptObserve:
    return "noexcept_observe";
  case ContractEvaluationSemantic::Prove:
    return "prove";
  case ContractEvaluationSemantic::Diagnose:
    return "diagnose";
  }
  return "invalid";
}

// Values match std::contracts::detection_mode per [support.contract.enum].
enum class ContractDetectionMode {
  PredicateFailed = 1,
  ExceptionRaised = 2
};

/// Indicates whether the contract-scope information was pushed before the
/// function's declaration context was available (and so needs special handling
/// when adjusting the context).
enum class ContractScopeOffset { ParentContext, FunctionContext };

/// Source type for a contract configuration entry.
enum class ContractConfigSourceKind {
  GroupSemantic,
  JSONInline,
  JSONFile,
};

/// The parsed P3595 configuration storage.  Its layout (in particular
/// ContractConfigEntry) evolves as new match criteria are added, so it is
/// defined in the impl-only header clang/Basic/ContractConfig.h and held by
/// ContractOptions behind a pointer -- keeping it out of the include graph of
/// everything that pulls in LangOptions.h.
struct ContractConfigData;
struct ContractConfigSource;

/// P3595 dynamic (link-/run-time) selection: the "output.dynamic" descriptor
/// of a matched contract config entry.  Companion to
/// ContractOptions::resolveContractSemantic (see resolveContractDynamic).
/// `Found` is false when the matched entry has no "dynamic" descriptor, or
/// when resolving in a constant-evaluation context -- a dynamic selector is
/// never invoked at compile time, so no descriptor is surfaced there (the
/// entry's Semantic is used instead; see the constant-evaluation rule in
/// resolveContractSemantic's implementation).
/// The linkage a P3595 dynamic selector is named with: a qualified C++ name,
/// which is mangled, or a verbatim C symbol.
enum class ContractDynamicLinkage : uint8_t { CXX, C };

struct ContractDynamicResult {
  std::string Name;
  // Meaningful only when Found.
  ContractDynamicLinkage Linkage = ContractDynamicLinkage::CXX;
  bool ProvideWeak = false;
  bool Found = false;
  // The matched entry's own "semantic", before it is adjusted to the allowed
  // set: what the selector's weak default returns (D3595R1).
  // InvalidContractEvaluationSemantic when the entry gives none, which is
  // never the case when ProvideWeak.
  ContractEvaluationSemantic Configured = InvalidContractEvaluationSemantic;
};

/// The throwing semantics: observe and enforce, the two whose evaluation may
/// let an exception leave the check -- the violation handler, which may
/// throw, is called through an entry point that lets it, as is the report of
/// an exception from the predicate.  No other semantic can: noexcept_observe
/// and noexcept_enforce (P4298) call the handler as noexcept, so an exception
/// from it terminates the program; quick_enforce traps inline and calls none;
/// ignore and assume evaluate nothing at run time (on the P3595 dynamic path
/// assume is evaluated as ignore), and neither do the P4405 prove and
/// diagnose.
inline constexpr unsigned ThrowingContractSemanticsMask =
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Observe)) |
    (1u << static_cast<unsigned>(ContractEvaluationSemantic::Enforce));

/// P3595, DECISIONS.md E137 and E138: true if \p Mask, the semantics a
/// dynamically selected contract may be evaluated with -- its label's
/// allowed_semantics facet intersected with the flag-gated base set -- holds
/// no throwing semantic (ThrowingContractSemanticsMask).  A dynamic selection
/// must not then bring one in: the best-fit clamp prefers a noexcept
/// counterpart, a compute_semantic result outside the set (an observe or
/// enforce included) is an invalid selection (precomputeDynamicTable), and an
/// invalid selection is reported, in the dispatch in
/// EmitContractStmtAsFullStmt, through the noexcept_enforce entry point
/// rather than the enforce one with -fcontracts-p4298, and without it, where
/// noexcept_enforce is not available, as a quick_enforce trap calling no
/// handler, whatever the label allows.  So no exception leaves the check.
/// Never for a contract with no label, nor in C, which has no labels: the
/// base set always holds both throwing semantics.  (gnu_gcc's
/// contract_semantics_nothrow_p.)
constexpr bool contractSemanticsNothrow(unsigned Mask) {
  return !(Mask & ThrowingContractSemanticsMask);
}

/// Per-assertion query for P3595 contract config resolution.
/// Namespace and location are lazily computed from the DeclContext
/// and SourceLocation to avoid cost when no config entry needs them.
struct ContractQuery {
  ContractKind Kind;
  bool CallerSide = false;
  bool InConstantEvaluation = false;
  unsigned AllowedMask = AllContractSemanticsMask;
  /// D4405: the semantic the assertion-control object's analysis_semantic
  /// facet gives the analysis evaluation, which takes the stronger of it and
  /// its configured semantic; ignore when there is none.  Read only for a
  /// callee-side query outside constant evaluation.
  ContractEvaluationSemantic LabelAnalysis = ContractEvaluationSemantic::Ignore;
  llvm::ArrayRef<std::string> Groups;

  const DeclContext *FnContext = nullptr;
  SourceLocation Loc;
  const SourceManager *SM = nullptr;

  llvm::StringRef getNamespace() const;
  llvm::StringRef getLocationFile() const;
  int getLocationLine() const;
  int getLocationColumn() const;

private:
  mutable std::string CachedNamespace;
  mutable std::string CachedLocationFile;
  mutable int CachedLocationLine = 0;
  mutable int CachedLocationColumn = 0;
  mutable bool NamespaceComputed = false;
  mutable bool LocationComputed = false;
};

/// D4405: the result of resolving a ContractQuery (D4405R0, "Adjusting to the
/// Allowed Semantics"; ContractOptions::resolveContract).
struct ContractResolution {
  /// The semantic of the runtime evaluation queried, adjusted to its allowed
  /// set (contractRuntimeAllowedMask), never prove or diagnose; for a
  /// constant evaluation, the semantic it uses
  /// (contractConstantEvaluationSemantic).  InvalidContractEvaluationSemantic
  /// if resolution failed.
  ContractEvaluationSemantic Runtime = InvalidContractEvaluationSemantic;
  /// The semantic of the analysis evaluation -- ignore, diagnose or prove --
  /// adjusted to its allowed set (contractAnalysisAllowedMask).  For a
  /// callee-side query it is the assertion's; for a caller-side one only what
  /// the caller-side lookup moved into it; always ignore in constant
  /// evaluation.  InvalidContractEvaluationSemantic if resolution failed.
  ContractEvaluationSemantic Analysis = ContractEvaluationSemantic::Ignore;
  /// Runtime, or InvalidContractEvaluationSemantic if either resolution
  /// failed.  The runtime evaluation is emitted with it; the analysis
  /// evaluation, the first, is placed on its own (a precondition's at each
  /// call, a postcondition's or assertion's in the body ahead of the check).
  ContractEvaluationSemantic Semantic = InvalidContractEvaluationSemantic;
  /// Whether the matched runtime entry has a dynamic selector (never in
  /// constant evaluation).
  bool Dynamic = false;
};

/// The contract options for a translation unit: the default evaluation
/// semantic, the P3100 assume gate, and the P3595 configuration sources that
/// together decide which evaluation semantic each contract assertion gets.
///
/// A contract group is a string of identifiers joined by '.', e.g. "a.b.c" is
/// a group with three subgroups.  A group's semantic is set with
///   -fcontract-group-evaluation-semantic=<group>:<semantic>
/// and the entries are tried in order, the first match winning (P3595), so
///   -fcontract-group-evaluation-semantic=std.hardening:enforce,std:ignore
/// enforces 'std.hardening.foo' while ignoring 'std.baz'.  Richer selection
/// (by kind, namespace, source location, caller) comes from the P3595 JSON
/// configuration supplied via -fcontract-configuration= or
/// -fcontract-configuration-file=; see resolveContractSemantic.
class ContractOptions {
public:
  ContractOptions() = default;

  using DiagnoseGroupFunc =
      std::function<void(ContractGroupDiagnostic, StringRef, StringRef)>;

  /// AllowColon admits ':' in the name, as a P3100 group name (ub:...) named
  /// by -fcontract-group-evaluation-semantic= has one.
  static bool validateContractGroup(llvm::StringRef GroupAndValue,
                                    const DiagnoseGroupFunc &Diagnoser,
                                    bool AllowColon = false);

  /// Validate a -fcontract-group-evaluation-semantic= argument, reporting
  /// problems through Diagnoser.  A bare "semantic" (no group) sets
  /// DefaultSemantic; a "group:semantic" entry is validated here and recorded
  /// as a configuration source (see addConfigSource) rather than stored
  /// separately.
  void addUnparsedContractGroup(StringRef GroupAndValue,
                                const DiagnoseGroupFunc &Diagnoser);

  /// The default semantics for contracts (the catch-all used by initConfig).
  /// Set by -fcontract-evaluation-semantic= (marshalled) or a bare
  /// -fcontract-group-evaluation-semantic=<semantic>.
  /// Whether the P4298 noexcept semantics are available (-fcontracts-p4298);
  /// without it they are downgraded, and a configuration naming one says so.
  /// Set by ParseLangArgs.
  bool NoexceptSemanticsAvailable = true;

  /// Whether the P4405 prove and diagnose semantics are available
  /// (-fcontracts-p4405); without it they resolve as ignore, and a
  /// configuration naming one says so.  Set by ParseLangArgs.
  bool ProvingSemanticsAvailable = true;

  ContractEvaluationSemantic DefaultSemantic =
      ContractEvaluationSemantic::Enforce;

  /// P3100: whether the "assume" evaluation semantic may actually be used.
  /// Set by -fcontracts-allow-assume.  When false, the query builders omit
  /// the assume bit from a contract's allowed set, so a resolved "assume" is
  /// adjusted to "ignore" (see resolveContractSemantic).
  bool AllowAssume = false;

  /// D4405: -fcontract-analysis-semantic=, the semantic of the analysis
  /// evaluation of every contract assertion: an entry
  /// {"match": {"analysis": true}, "output": {"semantic": S}} after those of
  /// the configuration sources (initConfig).  Ignore, the default, adds
  /// none, which is the same: no analysis entry matching is ignore.
  ContractEvaluationSemantic AnalysisSemantic =
      ContractEvaluationSemantic::Ignore;

  /// P3595 configuration, D4405 resolution: the semantics of the runtime
  /// evaluation \p Query describes and of the analysis evaluation, steps 1 to
  /// 5 of D4405R0's "Adjusting to the Allowed Semantics" (the label's
  /// compute_semantic, step 6, is Sema's).
  ContractResolution resolveContract(const ContractQuery &Query) const;

  /// P3595 configuration: resolve the evaluation semantic for a contract,
  /// the one code generation acts on (resolveContract's Semantic).
  /// InvalidContractEvaluationSemantic when the query's allowed set is empty
  /// (see clampToAllowed); callers diagnose it.
  ContractEvaluationSemantic
  resolveContractSemantic(const ContractQuery &Query) const;

  /// Clamp a candidate evaluation semantic into an allowed set: apply the
  /// P3100 "assume selected but disallowed -> ignore" rule, then the generic
  /// fallback order ({observe, enforce, quick_enforce, ignore}).  The P4405
  /// prove and diagnose share ignore's level: prove tries prove, diagnose,
  /// ignore; diagnose tries diagnose, ignore, prove; any other candidate
  /// reaching that level tries ignore, diagnose, prove.  So every semantic in
  /// \p AllowedMask is reachable.  Returns InvalidContractEvaluationSemantic
  /// -- never a semantic outside \p AllowedMask -- when \p Candidate is not
  /// a semantic or \p AllowedMask is empty.  Shared by
  /// resolveContractSemantic and the P3595 dynamic transform-table
  /// precomputation so both clamp identically.  Static: it needs no config.
  static ContractEvaluationSemantic
  clampToAllowed(ContractEvaluationSemantic Candidate, unsigned AllowedMask);

  /// P3595 dynamic selection: resolve the "output.dynamic" descriptor for a
  /// contract, running the same match scan (and the same
  /// constant-evaluation skip rule) as resolveContractSemantic.  This is a
  /// companion query rather than a change to resolveContractSemantic's
  /// return type so its existing call sites need not change; a caller that
  /// also needs the dynamic descriptor calls both.
  ContractDynamicResult
  resolveContractDynamic(const ContractQuery &Query) const;

  /// Add a configuration source in command-line order.
  void addConfigSource(ContractConfigSourceKind Kind, llvm::StringRef Arg);

  /// The configuration sources recorded in command-line order.  Empty when
  /// no configuration has been supplied.
  llvm::ArrayRef<ContractConfigSource> getConfigSources() const;

  /// Build the resolved entry list from the configuration sources, reporting
  /// any problems through Diags and reading any configuration files through
  /// VFS.  Must run before any contract is resolved: CompilerInstance calls it
  /// before semantic analysis, and Sema's constructor for a frontend that
  /// bypasses CompilerInstance.  Runs once; later calls are no-ops.
  void initConfig(DiagnosticsEngine &Diags, llvm::vfs::FileSystem &VFS) const;

  /// True iff the effective configuration contains an entry that requests
  /// caller-side contract checking with a semantic other than "ignore".
  /// Clang does not yet emit caller-side checks (see P3595); callers of
  /// this predicate use it to warn that such configuration is a no-op.
  bool configRequestsCallerSideChecks() const;

  /// The "C++"-linkage P3595 dynamic selector names the effective
  /// configuration contains, each once, in configuration order.
  llvm::SmallVector<llvm::StringRef> getCXXDynamicSelectorNames() const;

private:
  /// Parse JSON text and append config entries. SourceDesc is for diagnostics,
  /// which are reported through Diags.
  void parseConfigJSON(DiagnosticsEngine &Diags, llvm::StringRef JSON,
                       llvm::StringRef SourceDesc) const;

  /// Read a JSON file (through \p VFS) and parse it as contract configuration.
  void parseConfigFile(DiagnosticsEngine &Diags, llvm::vfs::FileSystem &VFS,
                       llvm::StringRef Path) const;

  /// Lazily-allocated parsed configuration (see ContractConfigData).  Shared
  /// so that copies of ContractOptions (LangOptions is copied) share the
  /// post-init, effectively-immutable configuration rather than deep-copying
  /// it.  Null until the first configuration source is added.
  mutable std::shared_ptr<ContractConfigData> Config;

  /// Ensure Config is allocated; returns it.
  ContractConfigData &ensureConfig() const;
};

} // namespace clang

#endif // LLVM_CLANG_BASIC_CONTRACT_OPTIONS_H
