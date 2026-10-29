---
id: 4930-p4405-analysis
subject: '[clang][contracts][P4405] the analysis evaluation'
depends: [4920-p4405-shadow]
regenerates: []
fixes: []
---

## Rationale
Give each contract assertion an analysis evaluation, as D4405R0 now
proposes: a separate evaluation, the only one `prove` and `diagnose` ever
apply to, whose semantic -- `ignore` (the default, no analysis),
`diagnose` or `prove` -- is selected independently of the semantics of the
assertion's other, runtime, evaluations.

Configuration.  A P3595 entry whose `match` has `"analysis": true` selects
the analysis evaluation's semantic, in a lookup of its own through the same
list: only such entries match it, and every other lookup skips them
(`"analysis": false` is the same as no key).  With `"constexpr": true` such
an entry never matches; with a `caller` key, or with an output that has a
`dynamic` selector or a semantic other than `ignore`, `diagnose` and
`prove`, it is a configuration error, and is skipped.  Constant evaluation
is never analyzed, so an entry with `"constexpr": true` whose semantic is
`prove` or `diagnose` is a configuration error too.
`-fcontract-analysis-semantic=<s>` (`ContractOptions::AnalysisSemantic`,
marshalled) stands for such an entry after those of the configuration
sources, and without `-fcontracts-p4405` gets the P4405 no-flag warning and
resolves as `ignore`, there being no `prove` or `diagnose` in any allowed
set.  C shares the configuration.

Resolution (D4405R0, "Adjusting to the Allowed Semantics").  The allowed
set of an assertion splits into its runtime part R and its analysis part
N, `prove` and `diagnose` (`contractRuntimeAllowedMask`,
`contractAnalysisAllowedMask`): the analysis evaluation may be `ignore`
only when R is not empty; a callee-side runtime evaluation is limited to
R, or to `ignore` when R is empty -- so only a label, never the
configuration, lets a callee-side check be skipped; a caller-side one may
always be `ignore`; with R and N both empty there is no valid semantic.
`ContractOptions::resolveContract` then (1) looks up both evaluations, (2)
moves a configured runtime `prove` or `diagnose` into the analysis
evaluation, which takes the stronger (`ignore` < `diagnose` < `prove`),
leaving the runtime evaluation `ignore`, (3) strengthens the analysis
evaluation with the label's `analysis_semantic` facet, (4) adjusts it to
its allowed set, and (5) adjusts the runtime semantic to its set, which
never holds `prove` or `diagnose`.  Sema adds step (6), `compute_semantic`
(`applyLabelFacets`): a `prove` or `diagnose` result moves to the analysis
evaluation, which must allow the stronger, and the runtime evaluation
becomes `ignore`, which must be in its own set; any other result must be
in the runtime set (`p4405-analysis-compute.cpp`).  The analysis
evaluation is resolved for the callee-side query only; a caller-side
query's analysis is just what its own step 2 moves in, and in constant
evaluation there is none: step 2 does not apply there, a constant
evaluation is adjusted to the whole allowed set, and a `prove` or
`diagnose` it reaches, however (an entry without the `constexpr` key, a
flag, a label's set or `compute_semantic`, whose result is accepted if it
is in the allowed set or the runtime one), is evaluated as `enforce` or
`observe` (`contractConstantEvaluationSemantic`).  The default semantic of
a dynamic selection is the run-time evaluation's own and is not moved by
step 2, and a dynamic selector's value is adjusted as a runtime
evaluation's (`precomputeDynamicTable`).

Each contract assertion carries both semantics: a `ContractStmt` holds the
label's analysis semantic (serialized, like the allowed set, so a PCH's
contracts are resolved with it) and the resolved ones, callee-side
(`ensureAnalysisSemantic`) and what the caller-side resolution moves into
the analysis evaluation at a call (`ensureCallerAnalysisSemantic`).

Placement.  The analysis evaluation is the assertion's first evaluation,
emitted on its own, ahead of every runtime evaluation, which is emitted
with its runtime semantic -- never `prove` or `diagnose` (an assertion in
`emitCheckForSemantic`).  A precondition's is made at each call, in the
caller (`DiagnoseCallerSideProvingContracts`), with the stronger of its own
semantic, resolved as for the function's definition, and what the
caller-side resolution moved into it; a postcondition's at a call is only
the latter.  The call's arguments are substituted for the parameters, as
for caller-side proofs before: at a call, a virtual call (for the function
it names), a call in a checked predicate, a constructor call (an inheriting
constructor's inherited one, whose `this` is not the object), the
constructor calls of an array new-expression, and an implicit destructor
call -- an automatic variable or a temporary going out of scope, a
delete-expression (also through P3097's wrapper), a member or base
destroyed -- whose site is an expression standing for the call
(`getContractImplicitCallSite`, `noteContractDestroySite`,
`DiagnoseImplicitDtorProvingContracts`, from `EmitCXXDestructorCall`).
The destructor of an aggregate's subobject is potentially invoked by the
aggregate's initialization ([dcl.init.aggr]): once a later subobject is
initialized by the initializer or a default member initializer, the call
that would destroy an already-built base, member, capture or array element
is analyzed, whether or not the partial-destruction cleanup is emitted --
it is not when nothing after the subobject can throw -- and nothing is
emitted for it (`PartialDtorAnalysis` in `CGExprAgg.cpp`,
`DiagnosePartialAggregateDtorProvingContracts`).  A
constructor or destructor call gives a predicate naming `this` its object
(`CodeGenFunction::ContractCallObject`); the proof shadow now defers such
calls, and a lambda's call operator whose predicate names no capture
(folding likewise), and maps a static `operator()`'s arguments past its
object argument (CLANG-854: folding gave each parameter the argument before
its own).  A postcondition's and a `contract_assert`'s analysis evaluations
are made in the function body (`EmitContractStmtAsFullStmt`), ahead of the
runtime check and the assumption a proof shadow takes from it, so a proof
never relies on the assertion's own check; an `ignore` runtime evaluation
after them places nothing.  No analysis evaluation in a P3097 wrapper or at
a call that evaluates its callee's interface contracts itself
(`InContractInterfaceEvaluation`), and none for a precondition in its
function -- in C++; C knows a function's preconditions only where it is
defined, so a C precondition is analyzed in its body (`isContractAnalyzedHere`).
A sound proof under `-fcontracts-prove-assume` is an assumption where its
obligation was, so it drops the checks after it in the same code
(`p4405-analysis-drop.cpp`); a caller's never reaches a callee-side check
not inlined, and a callee-side runtime evaluation is `ignore` only when R is
empty.

Indirect calls.  A call through a pointer or a reference cannot identify
the function, so it has no analysis evaluation, and a precondition can be
proven for it only if it is tautological.  Taking the address of a function
-- the unary `&`, the function-to-pointer conversion, a static member
through an object, a pointer to member function, a reference bound to it,
a lambda converted to a pointer to function (its call operator's address)
-- is diagnosed only where it is known that calls through the address need
a proof: a precondition whose labels allow no runtime semantic and whose
analysis evaluation is `prove` or `diagnose`, which folding with nothing
known of the arguments does not prove (none under
`-fno-contracts-prove-fold`).  "Contract assertion cannot be proven (or
predicate is always false) for a call through the address of 'F'" is an
error under `prove` and a `-Wcontract-unproven` warning under `diagnose`
(C++ only); the first unproven `prove` precondition is reported, else the
first `diagnose` one.  A precondition that only the configuration analyzes
is not diagnosed: that translation unit's configuration does not determine
what the calls through the address are.
`Sema::CheckContractAddressTaken` walks each full-expression for the
functions it names other than as a call's callee, skipping templates,
unevaluated operands, the bodies of lambdas and statement expressions
(checked on their own) and a lambda's conversion function; a default
argument is checked where it is written ([dcl.fct.default]), or for a
template's as it is instantiated, not at the calls that use it.  The
definition of an object of static or thread storage duration takes the
address of its destructor, which the implementation calls at exit or
thread end, and is diagnosed in the same way, with a note saying so
(`Sema::CheckContractStaticDestructor`, from `FinalizeVarWithDestructor`;
not for `no_destroy` or constant destruction), and so is a temporary whose
lifetime is extended to static or thread storage duration, at the
temporary (`Sema::CheckContractStaticTemporary`, from lifetime extension in
`CheckExprLifetime.cpp`).  The destructor calls the implementation
generates for a static array's elements are analyzed like any other, so
such an array also gets "cannot be proven" in code generation
(`p4405-analysis-static-array-dtor.cpp`).

The label facet: `analysis_semantic_label`, a control object whose
`analysis_semantic()` is callable on a const object with no argument and
converts to `evaluation_semantic`; its value must be a constant `ignore`,
`diagnose` or `prove`, or the program is ill-formed
(`labelAnalysisSemantic`, `p4405-analysis-facet-invalid.cpp`).  A combined
label models it when either side does and returns the stronger of the two.
libc++'s `terminating` also allows `prove`, and `symbolic` `diagnose` and
`prove`; `all()` and the `never_` labels already hold every enumerator.

Tests: `p4405-analysis-*.cpp` (configuration errors, the flag, the opt-in
lookups, labels, the facet, `compute_semantic`, placement, order, drops,
the address-of rule and lambda conversions, default arguments, static
objects' and temporaries' destructors, constructor (with the elided temporaries of a
prvalue), destructor and class-returning calls, the manifestly
constant-evaluated contexts and constant destruction, which have none, an
elided temporary's destructor call, which has none either, and the
cleanup of an aggregate member built before the initialization throws,
analyzed also when nothing after it can throw, with the code unchanged),
`Runnable/p4405-analysis-runtime-run.cpp`, `-elided-temporary-run.cpp`,
`-elided-temporary-aggregate-run.cpp`,
`p4405-constexpr-enforce.cpp`
and `p4405-constexpr-matrix-*.cpp` (constant evaluation's `enforce` and
`observe` under each label and configuration, in both constant
evaluators), `p4405-pch-label-analysis.cpp`, and libc++'s
`p4405_analysis_semantic_label.compile.pass.cpp`.

Most of the change teaches existing machinery, and those lines sit in the
commits that wrote it: the configuration's parsing, matching and
resolution, `ContractResolution`, the configuration errors and the no-flag
warning are `2400-p3595`'s, as are `resolveContractConfig` and
`precomputeDynamicTable`; the split-set helpers and the other semantic
helpers in `ContractOptions.h` are `1000-p2900-base-basic`'s, with the
enumerators; the `ContractStmt` fields and their serialization are
`1020-p2900-base-ast`'s and `1120-p2900-base-serialization`'s; the constant
evaluators' comments `1090`'s and `4000-p3098`'s; `applyLabelFacets`'s
steps, the libc++ concept, the combined label's member and the
`terminating` and `symbolic` sets `4200-p3400-core`'s; the `std` module
export `1160-p2900-base-libcxx`'s; the driver's forwarding
`6900-cross-paper-options`'s.  Likewise for the placement: the body's
analysis evaluation sits in `EmitContractStmtAsFullStmt`
(`1100-p2900-base-codegen`), the
call's in `DiagnoseCallerSideProvingContracts` and the constructor hook
(`4900-p4405`), the wrappers' flag in P3097's functions (`3000-p3097`),
the shadow's substitution and the members and declarations beside its own
(`4920-p4405-shadow`), folding's (`4910-p4405-fold`, with
`p4405-fold-static-operator.cpp`), the caller-side analysis cache
(`1020-p2900-base-ast`).  This commit holds what is the analysis
evaluation's alone: the flag, its documentation, the facet's detection and
its diagnostic, the implicit destructor calls' sites and hooks, the array
new-expression's hook, the address-of rule and its diagnostics, and the
tests.  The C tests ride with `5000-p4299`.

## Compile gap
**Compile gap: an earlier commit calls into this one.**
`4200-p3400-core`'s `applyLabelFacets` calls `labelAnalysisSemantic`,
defined here, which reports `err_contract_analysis_semantic_invalid`, also
defined here; so from `4200-p3400-core` until this commit `SemaContract.cpp`
does not compile.  The call cannot be cut out: it is a line inside that
function, and the label's analysis semantic runs through the whole
resolution.  Likewise `2400-p3595`'s no-flag warning in
`CompilerInvocation.cpp` names `OPT_fcontract_analysis_semantic_EQ`, which
this commit's `Options.td` record declares.

**Compile gap: earlier commits use this one's functions.**  The
destructor hook in `EmitCXXDestructorCall` is here, but `1100`'s
`EmitContractStmtAsFullStmt` calls `isContractAnalyzedHere`, `4900`'s `DiagnoseCallerSideProvingContracts`
reads `ContractCallObject` and `ensureCallerAnalysisSemantic`, declared with
`4920`'s and `1020`'s blocks, and `3000`'s wrappers set
`InContractInterfaceEvaluation`.  **Behaviour gap:** from `4900-p4405` to
this commit a callee-side `prove` or `diagnose` is not diagnosed in the
function (the early exit that did is gone, replaced here by the analysis
evaluation).

**Behaviour gap: the driver does not pass the flag on before
`6900-cross-paper-options`**, whose contracts block in `Clang.cpp` forwards
it; until then only a `-cc1` command line sees it, and the driver line of
`p4405-analysis-flag-invalid.cpp` cannot pass.

## Contents
- clang/include/clang/Options/Options.td : /^def fcontract_analysis_semantic_EQ\b/
- clang/include/clang/Basic/DiagnosticSemaKinds.td : /\bdef\s+err_contract_analysis_semantic_invalid\b/, /\bdef\s+err_contract_address_unproven\b/, /\bdef\s+warn_contract_address_unproven\b/, /\bdef\s+note_contract_static_destructor_address\b/
- clang/include/clang/Sema/Sema.h : /^  \/\/\/ D4405 \(SemaContract\.cpp\): diagnose each function whose address the$/
- clang/lib/Sema/SemaContract.cpp : /^labelAnalysisSemantic\(Sema &S, Expr \*LabelExpr, QualType LabelTy,$/, @diagnoseContractAddressTaken, /^enum class AddressTakenProof \{ Proven, Unproven, AlwaysFalse \};$/, /^lambdaConversionCallOperator\(const CXXConversionDecl \*Conv\) \{$/, @CheckContractAddressTaken, @CheckContractStaticDestructor, @CheckContractStaticTemporary
- clang/lib/Sema/CheckExprLifetime.cpp : /^        SemaRef\.CheckContractStaticTemporary\(MTE\);$/
- clang/lib/Sema/SemaExprCXX.cpp : /^  CheckContractAddressTaken\(FullExpr\.get\(\)\); \/\/ D4405$/
- clang/lib/Sema/SemaDeclCXX.cpp : /^  \/\/ D4405: a default argument that takes the address of a function is$/, /^  CheckContractStaticDestructor\(VD, Destructor\); \/\/ D4405$/
- clang/lib/CodeGen/CGContracts.cpp : @getContractImplicitCallSite, @noteContractDestroySite, @DiagnoseImplicitDtorProvingContracts, @getContractFallbackDestroySite, @DiagnosePartialAggregateDtorProvingContracts, @isContractAnalyzedHere, @emitCheckForSemantic:3
- clang/lib/CodeGen/CGClass.cpp : /^  \/\/ D4405: an implicit call's analysis evaluations\.$/
- clang/lib/CodeGen/CGDecl.cpp : /^  \/\/ D4405: its destructor call is diagnosed at the variable\.$/
- clang/lib/CodeGen/CGExpr.cpp : /^  noteContractDestroySite\(Slot\.getAddress\(\), E\); \/\/ D4405$/
- clang/lib/CodeGen/CGExprAgg.cpp : /^  CGF\.noteContractDestroySite\(Dest\.getAddress\(\), E\); \/\/ D4405$/, /\bPartial(DtorAnalysis)?\b/
- clang/lib/CodeGen/CGExprCXX.cpp : /^    DiagnoseCallerSideProvingContracts\(Ctor, CCE\); \/\/ P4405, as for a call$/, #22:1, /^  CGF\.noteContractDestroySite\(Ptr, DE\); \/\/ D4405$/
- clang/docs/UsersManual.md : #0, #0:0
- clang/test/Contracts/p4405-analysis-*.cpp : *
- clang/test/Contracts/p4405-analysis-*.json : *
- clang/test/Contracts/Runnable/p4405-analysis-*.cpp : *
- clang/test/Contracts/Runnable/p4405-analysis-*.json : *
- clang/test/Contracts/p4405-constexpr-enforce.* : *
- clang/test/Contracts/p4405-constexpr-matrix-*.cpp : *
- clang/test/Contracts/p4405-pch-label-analysis.cpp : *
- clang/test/Contracts/p4405-paper-*.cpp : *
- clang/test/Contracts/p4405-paper-*.json : *
- clang/test/Contracts/Runnable/p4405-paper-*.cpp : *
- clang/test/Contracts/Runnable/p4405-paper-*.json : *
- clang/test/Contracts/p3400-label-nondependent-template.cpp : *
- clang/test/Contracts/Runnable/p3400-label-lambda-specifier.cpp : *
- libcxx/test/std/contracts/p4405_analysis_semantic_label.compile.pass.cpp : *
