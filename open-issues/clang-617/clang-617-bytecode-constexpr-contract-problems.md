# CLANG-617: the bytecode constant interpreter does not report contract problems as M7-M9 require

**Status:** Open; deferred by the user 2026-09-30 (do not address it now).
**Kind:** deferred.
**Affects:** measured 2026-09-30 on the branch install at `58e651a5964d`, with
`-fexperimental-new-constant-interpreter`.  The tree evaluator (the default)
is correct since CLANG-611/614.
**Watch test:** `clang/test/Contracts/OpenBugs/bytecode-constexpr-contract-problems.cpp`.

## What is wrong

DECISIONS.md M7-M9 (notadragon `src/pubs/impl/p3850impl/DECISIONS.md`,
section P) say how contract problems met while constant-evaluating are
reported: a not-constant predicate is diagnosed (error under a terminating
semantic, warning under observe) only if the evaluation otherwise completes;
if the evaluation outside the predicates is not constant, every contract
diagnostic is discarded; every problem is reported at its own location; and
an array bound is judged like a `const size_t` initializer.

The bytecode interpreter (`clang/lib/AST/ByteCode/`, contract support in
`Compiler::visitContractStmt` and `ContractFailed`) has its own contract
implementation, which the CLANG-611/614 rework of `ExprConstant.cpp` did not
touch.  Under it:

- a not-constant predicate makes the evaluation fail instead of being
  reported once the evaluation completes, so `static_assert (h (g) == 1)`
  with a precondition reading a non-constant `g` is rejected under observe;
- only the first problem of an evaluation is reported;
- an observe warning can be issued more than once for one expression.

## Measurement

The `notional` group of the contract test matrix (466 cells), probed with
`CONTRACT_MATRIX_CLANG_FLAGS=-fexperimental-new-constant-interpreter`
(`contract-matrix-gen.py probe --group notional`): 216 cells mismatch, all
syntax-phase, all Clang; the same probe with the tree evaluator mismatches in
2 cells (a repeated warning, fixed at the same time as this was filed).

| mismatch | cells |
|---|---|
| error missing | 100 |
| error unexpected, warning missing (a not-constant predicate under observe) | 61 |
| warning missing | 22 |
| error unexpected | 14 |
| warning unexpected | 12 |
| warning repeated | 7 |

The watch test runs the tree evaluator's three M7-M9 regression tests
(`constexpr-contract-every-problem.cpp`, `constexpr-contract-array-bound.cpp`,
`constexpr-contract-observe-warning-once.cpp`) under the interpreter; all
three fail today.

## Other Contracts tests failing under the interpreter

Measured 2026-09-30 at `e551cc96e842` by running each Contracts test's plain
`%clang_cc1 ... -verify` RUN lines (239 of the 883 files have one that passes
today) with and without `-fexperimental-new-constant-interpreter`.  25 fail
under the interpreter only:

- M7-M9 reporting (this issue proper): `constexpr-contract-array-bound`,
  `constexpr-contract-every-problem`, `constexpr-contract-observe-not-constant-init`,
  `constexpr-contract-observe-warning-once`, `constexpr-observe-nonconstant-predicate`,
  and seven `matrix-notional-*` files (enforce/observe/quick-enforce errors,
  their not-constant variants, observe warnings).
- Contracts on constexpr constructors, destructors and virtual interfaces:
  `constexpr-ctor-contracts`, `constexpr-dtor-contracts`,
  `constexpr-virtual-interface-contracts`.
- Not yet diagnosed (some may be upstream interpreter differences, not ours):
  `Sema/expr.const.cpp`, `coroutine-compat`, `lambda-capture-scope-diagnostics`,
  `p3400-control-scope`, `parse-contract-grammar`, `postcondition-param-lambda`,
  `result-name-shadows-lambda-capture`, `noncopyable-capture-on-declaration`,
  `contract-control-using-qualified-in-label`, `constexpr-double-destroy`.

The other 644 files (driver, FileCheck and runnable RUN lines) were not
measured; the first step below covers them.

## A fold drops the P3100 null-indirection checks (CLANG-753)

Measured 2026-10-14 on the branch install at `77a97b5653f3` (M65-Clang).
The tree evaluator gives up on an indirection through a null pointer in an
evaluation that is not manifestly constant-evaluated
(`refuseP3100NullIndirection`, `ExprConstant.cpp`), so CodeGen does not fold
a constexpr call whose run-time `ub:expr.unary.dereference.nullptr` check
would fail.  The interpreter has no counterpart:
`Runnable/p3100-null-unaccessed-indirection-fold.cpp` built with
`-fexperimental-new-constant-interpreter` folds the `if` condition of 13 of
its 15 forms (`sp->sf()`, `(*sp).sf()`, `sp->ck`, `sp->E`, `&sp->k`,
`&p[0]`, `*p;`, `&*fp`, `*fp;`, `&sp->m`, `&sp->arr[1]`, `&pp[0][1]`,
`F &r = *fp`; not `&*p` or `int &r = *p`), so those checks never run, and
`__builtin_constant_p` of such a call is 1.

## Plan (when this is picked up)

The goal is that every Contracts test runs under both evaluators without
duplicating any test by hand, so a gap shows up as soon as it is introduced.

The switch already exists: `-f[no-]experimental-new-constant-interpreter`,
whose default is the CMake option `CLANG_USE_EXPERIMENTAL_CONST_INTERP`
(off).  Upstream's own approach, two RUN lines per file with
`-verify=ref,both` / `-verify=expected,both` prefixes where diagnostics
differ (about 490 upstream test files), is the manual duplication to avoid.
A second build with the CMake option on would run all of clang/test through
the interpreter but costs a second 2.5-hour tree; not worth it.

1. Add `clang/test/Contracts/lit.local.cfg` (about 20 lines): when a lit
   parameter is set, append `-fexperimental-new-constant-interpreter` to the
   `%clang_cc1`, `%clang` and `%clangxx` substitutions for the directory and
   add a lit feature (`contracts-bytecode`).  Add a `--contracts-bytecode`
   option to notadragon `bin/clang/test.sh`.  The directory runs in about 15
   seconds of lit time, so the second run is nearly free.
2. Run the whole Contracts suite both ways on the Space to get the complete
   failure list, including the files the measurement above could not run.
3. Triage each failure: ours (fix it) or an upstream interpreter limitation
   (`UNSUPPORTED: contracts-bytecode` with a comment, or `ref`/`expected`
   verify prefixes where only the diagnostics differ).  Record each in the
   ledger.
4. Fix this issue: port the tree evaluator's M7-M9 reporting (ExprConstant.cpp
   `ContractProblemReport`: report once the evaluation completes, every
   problem at its own assertion, observe warnings deduplicated per expression)
   into the interpreter's contract code (`Compiler::visitContractStmt`,
   `ContractFailed`).  When the watch test XPASSes, retire it.
5. Make the both-ways run part of the normal Contracts gate.

The switch alone is about an hour; the cost is in steps 3 and 4.
