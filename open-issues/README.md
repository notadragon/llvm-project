# Open Issues on This Branch

What is currently broken on our Clang contracts branch -- whatever its origin.
If you hit something while using this compiler, look here first: a row means
we already know, and tells you whether there is a way around it.

A row is removed, and any file it points to in this directory deleted, as soon
as the issue is **fixed on this branch**. Git history is the record; nothing
is archived in place.

**This file is the only ledger of owed work on this branch.** Every known
defect and deferral is a row in the first table, and every owed
cleanup (style, comments, dead code, duplication) or task (a missing test, an
owed run, a blocked chore) is a row in the task table. Review notes, plans and
research trails written elsewhere are records of reasoning, not lists of work:
an item found there that is not a row here has been lost. Record it here
first, then write anywhere else. Divergences are recorded in their own
table, [Divergences](#divergences), and are not owed work.

This is a different question from the one
[`../bug-reports/`](../bug-reports/README.md) answers. That directory tracks
bugs that reproduce on **stock upstream** Clang, including ones we have
already fixed here, and a row there survives until *upstream* fixes it. An
upstream bug we have not yet fixed appears in both; its single writeup lives
in `bug-reports/` and the row below links to it.

Contracts are not upstream in Clang, so a contracts-dependent bug has nothing
upstream to reproduce against and is not tracked in `bug-reports/`. Such
issues are branch-only by definition and their writeups live here.

**Next ID:** CLANG-688

## Layout

One directory per issue, `clang-NN/`, the same shape as
[`../bug-reports/`](../bug-reports/README.md) -- so a reader who has learned
one directory has learned the other, and an issue that turns out to
reproduce on stock can move across without being reshaped.

Nothing here is ever attached to anything upstream: a row in this file is
either branch-only or an upstream bug we have not fixed, whose report lives
in `bug-reports/`. So every file here keeps its prefix, and the
prefix-means-do-not-attach rule that governs `bug-reports/` has nothing to
decide in this directory.

IDs come from one sequence per compiler, shared with `bug-reports/` and with
the cleanup and task table, and are never reused. Allocate from the line above and increment it. Both tables
delete rows, so the highest ID visible in either is not a reliable counter.

`--` in the Upstream column means the issue is ours alone with nothing
upstream to link -- for a contracts-dependent issue that is always the case,
since contracts are not upstream in Clang. `None found (searched <date>)`
means GitHub Issues was searched and nothing matched; `UNKNOWN` means nobody
has looked.

**Kind** is one of `defect` (wrong, and we intend to fix it) or `deferred`
(known, currently out of scope) in the first table, `divergence` (our
compilers disagree and the standard does not clearly settle which is right)
in the [Divergences](#divergences) table, and
`cleanup` (no behaviour change: style, comments, naming, dead code,
duplication) or `task` (owed work that is neither a defect nor a cleanup) in
the task table; `cleanup` rows (no behaviour change: style, comments, naming,
dead code, duplication) live in [`CLEANUP.md`](CLEANUP.md). A `cleanup` or `task` row needs no writeup, reproducer or watch
test: its **Where** column names the file and symbol, and one line of summary
is enough. A defect row always has a writeup, and a watch test in both
compilers' suites (`clang/test/Contracts/OpenBugs/` here), xfailed per
measured behaviour.

**A new issue is always recorded as `defect` when it is discovered.** It
becomes `deferred` only when the user has explicitly said they do not want to
expand scope far enough to fix it -- never by an agent's own judgement that a
fix looks hard or invasive.

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| CLANG-5 | A by-value return object is never destroyed when a local's destructor throws after it is built | deferred | [#12658](https://github.com/llvm/llvm-project/issues/12658) | [../bug-reports/clang-05-retval-not-destroyed-on-throwing-cleanup.md](../bug-reports/clang-05/clang-05-retval-not-destroyed-on-throwing-cleanup.md) |
| CLANG-8 | `this` is accepted in the declaration of an explicit-object member function, where it is ill-formed | deferred | None found (searched 2026-09-05) | [../bug-reports/clang-08-this-in-xobj-declaration.md](../bug-reports/clang-08/clang-08-this-in-xobj-declaration.md) |
| CLANG-9 | Constant evaluation accepts forming a member's or non-virtual base's address before its constructor begins | deferred | [#211286](https://github.com/llvm/llvm-project/issues/211286) (partial) | [../bug-reports/clang-09-member-address-before-ctor.md](../bug-reports/clang-09/clang-09-member-address-before-ctor.md) |
| CLANG-71 | Route libc++ hardening assertions through contracts (keep the mode cascade and messages, no GNU statement expression, re-point `check_assertion.h`). Requested by the user 2026-08-12, design approved, implementation parked by the user 2026-08-13 | deferred | -- | `libcxx/include/__assert`, `libcxx/test/support/check_assertion.h` |
| CLANG-72 | Extension: an additional caller-side check when calling through a virtual function's interface. Deferred by the paper author as a possible future extension | deferred | -- | -- |
| CLANG-74 | Route LeakSanitizer reports through the contract-violation handler (assume/enforce), as ASan/UBSan/TSan/MSan are.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-75 | Sweep the remaining `-fsanitize=` checks (beyond ASan/UBSan/TSan/MSan) for contract routing.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-78 | A dynamic/label-selected semantic that can only yield nonthrowing results should make the contract not potentially-throwing.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-80 | Explore adding `pre` macros (pre/body idiom, as in BDE) to libc++/libstdc++ headers.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-86 | P3595: add function-level and module-name match criteria to configuration entries.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `contracts-config.cc:357`, `contracts-config.cc:593-632` |
| CLANG-88 | The remaining P3100 UB categories (Group C whole-program cases, VLA/flexible-array/C cases, and the ~35 with no locally checkable site); int->float, `[[noreturn]]` returns and pointer bounds are CLANG-91, CLANG-89 and CLANG-92 (was also CLANG-77).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-89 | `[[noreturn]]` function returns (`dcl.attr.noreturn.eventually.returns`) check.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-90 | Other null-pointer cases (`conv.ptr.virtual.base`, `expr.dynamic.cast.pointer.lifetime`, `expr.static.cast.downcast.wrong.derived.type`, `expr.mptr.oper.member.func.null`).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `expr.dynamic.c`, `expr.static.c` |
| CLANG-91 | Float conversion checks F.3.6 `conv.double.out.of.range`, F.3.8 `conv.fpint.int.not.represented`, F.3.17 `expr.static.cast.fp.outside.range`, including the open question of how to separate F.3.6 from F.3.17.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `conv.fpint.in`, `expr.static.c` |
| CLANG-92 | P3100: implement a check for pointer-arithmetic bounds (`expr.add.sub.diff.pointers`).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-95 | Consolidate consecutive location/namespace config entries into a map instead of a linear scan (performance, low priority).  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | `ContractOptions.cpp:737,765`, `contracts.cc:877,1189` |
| CLANG-98 | Measure, and maybe improve, a plain-semantic check inside a `noexcept` function using the ABI's localized terminate instead of unwind termination.  Deferred by the user 2026-10-10 (DECISIONS.md E51) | deferred | -- | -- |
| CLANG-108 | P3595 per-module configuration stored in the CMI (callee uses its module's configuration, caller the TU's).  Deferred by the user 2026-09-25: revisit when other work runs out; do not raise it before then | deferred | -- | -- |
| CLANG-111 | Explore supporting the `assume` semantic in more places (an optimiser assumption where sound); it lowers to `ignore` for now, which is valid and sometimes best.  Deferred by the user 2026-09-25.  Includes real codegen for `assume` on explicit contracts (was CLANG-87) | deferred | -- | `clang/lib/CodeGen/CGContracts.cpp` |
| CLANG-123 | The contracts exports (`__cxa_pure_virtual_*`, `__cxa_contract_violation_*`, the `std::contracts` functions, the libcontracts imports) are in `x86_64-unknown-linux-gnu.libcxxabi.v1.stable.exceptions.nonew.abilist` since M6 (generated by `generate-cxx-abilist`; `check-cxx-abilist` clean, notadragon_wg21 `bin/clang/test.sh --libcxx-abilist`), but absent from the other eight `libcxx/lib/abi/*.abilist` lists (Linux without exceptions, Darwin, FreeBSD, Android, AIX), which only a build of each configuration can produce; ABI-list CI fails on upstreaming until they are added.  Deferred by the user 2026-10-10 (DECISIONS.md E55) | deferred | -- | -- |
| CLANG-157 | Replace the CWG GitHub issue URL with "CWG issue NNNN" once numbered.  Deferred by the user 2026-10-10 (DECISIONS.md E55): until cplusplus/CWG issue #988 has a CWG number | deferred | -- | `Runnable/contract-retval-destroyed-on-unwind.cpp:15` |
| CLANG-472 | Cleanup: replace the `EvaluateContracts` parameter threaded through `EvalInfo` with a Sema RAII override (cor3ntin's review item D19; "later", per the user 2026-09-25).  Deferred by the user 2026-10-10 (DECISIONS.md E55) | deferred | -- | `include/clang/AST/Expr.h:758`, `lib/AST/ExprConstant.cpp:5974,22402` |
| CLANG-478 | Work that needs Clang reflection support: revisit on the first rebase that brings `-freflection` into Clang.  Deferred by the user 2026-09-25; the list is in the linked file | deferred | -- | [clang-478-needs-reflection.md](clang-478/clang-478-needs-reflection.md) |
| CLANG-617 | Under `-fexperimental-new-constant-interpreter`, contract problems in constant evaluation are not reported as DECISIONS.md M7-M9 require (a not-constant predicate fails the evaluation, only the first problem is reported, an observe warning can repeat), and about 25 Contracts tests fail under the interpreter; the default tree evaluator is correct.  Since M6 the interpreter also lets a contract predicate's modifications stand, where the tree evaluator undoes them (DECISIONS.md E8, CLANG-625): `Sema/basic.contract.eval.cpp` and `contract-constexpr-side-effect-rollback.cpp` fail under it.  Deferred by the user 2026-09-30; the writeup holds the plan (run the Contracts suite under both evaluators from one lit switch, then fix) | deferred | -- | [clang-617-bytecode-constexpr-contract-problems.md](clang-617/clang-617-bytecode-constexpr-contract-problems.md) |
| CLANG-628 | An out-of-class definition of a member of a class template takes its parameters' top-level cv-qualifiers from the in-class declaration, for parameters of dependent type (stock Clang too; not a contracts bug).  Deferred by the user 2026-10-07: a known upstream bug | deferred | [#29713](https://github.com/llvm/llvm-project/issues/29713) | [../bug-reports/clang-628/clang-628-param-cv-out-of-class-member.md](../bug-reports/clang-628/clang-628-param-cv-out-of-class-member.md) |
| CLANG-661 | clang-format has no contracts support (nor does upstream): `pre`/`post`/`contract_assert` format as calls, so simple forms come out right, but a result name's colon is spaced as a bit-field (`post(r : r > 0)`), each specifier on a broken line is indented one level deeper than the last, a P3098 capture list formats as a subscript (`post[old = x](...)`, even in GNU style), and an attribute breaks mid-specifier (`pre` / `[[deprecated]] (x < 100)`).  Needs the token annotator taught the contract keywords, break rules, and Format unit tests; worth offering upstream.  Deferred by the user 2026-10-09 | deferred | None found (searched 2026-10-09) | `clang/lib/Format/TokenAnnotator.cpp`, `clang/unittests/Format/` |
| CLANG-678 | P3400R4's dimension layer and `hardened` are not in libc++'s `<contracts>`: `dimensioned_label`, `dimension_list`, `is_dimensioned_label`, the label combination operator's rejection of overlapping dimensions, and the labels `opt`, `audit` and `hardened`.  Mirror of GCC-695; the rest of the surface was CLANG-585, done in M31-Clang.  Deferred by the user 2026-10-10 (notadragon_wg21 DECISIONS-resolved.md E59 (b)) | deferred | -- | `libcxx/include/contracts` |
| CLANG-679 | The rethrow-bypass analysis bails on a local handler defined out of line later in the translation unit (emitted before the handler's body exists); it would need the decision deferred to the end of the TU.  Mirror of GCC-696; the `switch` half was CLANG-94, done in M31-Clang.  Deferred by the user 2026-10-10 (notadragon_wg21 DECISIONS-resolved.md E62 (b): no delayed evaluation) | deferred | -- | `clang/lib/CodeGen/CGContracts.cpp` (`RethrowAnalysis`) |

## Divergences

Where our compilers (GCC, Clang, and the EDG front end) behave differently
and the standard does not clearly settle which is right. The divergence is
accepted: a row records it so that nobody rediscovers it, and is not owed
work. A row is deleted if the compilers come to agree. Same ID sequence as
the table above.

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| CLANG-2 | Under `-fsanitize=bool -fsanitize-recover=bool`, an invalid `_Bool` load keeps its raw bits and reads as true; GCC coerces it to `false`.  Accepted by the user 2026-10-10 (DECISIONS.md E57) | divergence | None found (searched 2026-09-05) | [../bug-reports/clang-02-ubsan-bool-recover.md](../bug-reports/clang-02/clang-02-ubsan-bool-recover.md) |
| CLANG-683 | A precompiled header made under a different contract configuration (`-fcontract-evaluation-semantic=`, or a configuration file with other contents) than the translation unit's: Clang records no configuration in the PCH and uses it with the TU's configuration, while GCC records the configuration and rejects the PCH (gnu_gcc GCC-702, where the row is written up) and the EDG front end does not use it (EDG-129).  In none of the three does the header's configuration override the TU's; PCHs are outside the standard.  Recorded 2026-10-11 (M37-GCC) | divergence | -- | `clang/test/Contracts/pch-contract-semantic.cpp`, `pch-contract-config-file-changed.cpp` |
| CLANG-684 | A contract assertion repeated on a later declaration (the definition) and violated: Clang reports the definition's assertion, except for a member of a class template whose return type is not deduced, where it reports the in-class declaration's -- in constant evaluation and in `contract_violation::location()` alike.  GCC reports the definition's in every case (gnu_gcc GCC-703); the EDG front end the first declaration's, except that through edg-gxx a template is put out as written and g++ reports the definition's (edgcpp EDG-130, where the row is written up).  The location is implementation-defined and the assertions are equivalent; whether Clang should be consistent is notadragon_wg21 DECISIONS.md E69.  Recorded 2026-10-11 (M38-EDG) | divergence | -- | `clang/test/Contracts/contract-repeated-location.cpp` (`location()`, observe) and `contract-repeated-location-constexpr.cpp` (constant evaluation, enforce): five shapes, pre and post; also `contract-classtemplate-pack-member-fold-constexpr.cpp` (constant evaluation, deduced return type) |

## Tasks

Owed work that is not a defect and not a cleanup: a missing test, an owed
run, a decision owed, unimplemented scope. Same ID sequence as the table
above. Cleanups live in [`CLEANUP.md`](CLEANUP.md).

| ID | Summary | Kind | Where |
|----|---------|------|-------|
| CLANG-687 | Mirror GCC-706 and GCC-707.  GCC ICEd on a contract predicate spanning a line of only blanks (PR127857), and got the comment wrong whenever a predicate began or ended inside a macro expansion; both are fixed there, and Clang gets both right (probed 2026-10-12 over sixteen macro-boundary shapes).  GCC's rule now matches Clang's except for a contract written by a macro: GCC gives the predicate (`MY_PRE (x < 0)` gives `x < 0`, `PRE_NEG` gives `x < 0`), Clang the invocation (`MY_PRE (x < 0)`, `PRE_NEG`) -- a divergence recorded in gnu_gcc's open-issues (GCC-708), not a defect.  Add test-only mirrors of gnu_gcc's `g++.dg/contracts/cpp26/contract-comment-blank-line.C` (no crash across a line of spaces, of a tab, two in a row, for every kind of contract and a member function's late-parsed contracts; pin Clang's own text, which keeps the line breaks) and `contract-comment-macro.C` (every macro shape, pinning Clang's text, including the wrapper-macro shapes where it differs), plus `-ftrack-macro-expansion=0` if Clang has an equivalent | task | `clang/test/Contracts/` |
