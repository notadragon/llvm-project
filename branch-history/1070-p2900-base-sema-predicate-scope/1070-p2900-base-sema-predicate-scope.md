---
id: 1070-p2900-base-sema-predicate-scope
subject: '[clang][contracts] predicate scope: constification, odr-use and result names'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
What a contract predicate may name, and what naming it means:
constification, the lambda-capture rules, and the odr-use rules.

This is split out from `1060-p2900-base-sema-core` because it is a
different question.  That commit answers "is this contract-specifier
well-formed and does it agree with the other declarations of its
function"; this one answers "inside the predicate, what does an identifier
denote".  Almost none of it is in `SemaContract.cpp` -- it is in
`SemaExpr.cpp`, `SemaLambda.cpp`, `SemaLookup.cpp` and `Scope.h`, because
the rules are modifications to how ordinary name handling behaves in one
context.

**Constification is the centre of it.**  [basic.contract.general] makes an
id-expression naming a variable, a parameter or `this` behave as
const-qualified inside a predicate, so that a predicate cannot write to
what it is describing.  `getContractConstification` is the one place that
decides; `DeclRefExpr` grows `isConstified()` and `isInContractContext()`
so the answer travels with the expression, and the assignment diagnostics
consult those rather than re-deriving the context.  The `note_contract_context`
note exists because the error is otherwise unexplainable -- a variable is
const here and not three lines away, and the note is what points at the
`pre` that made it so.

**`ContractAssertScope` is a scope flag that deliberately does not nest.**
The comment on it says why: a predicate is its own full-expression, and the
constification and result-name rules apply to that predicate alone, not to
a lambda body or statement-expression written inside it.  Every other scope
flag describes everything nested within it, so this one reads as an anomaly
until you know that.  It is bit 32, which is what forces `ScopeFlags` to
`uint64_t` -- as `unsigned long` the enumerator was not representable in
the enum's own fixed underlying type and Clang did not build on Windows at
all.  The `static_assert` beside it now checks the width; the signedness
assertion that was already there did not catch this.

**Captures are where the rules interact badly and the tests are dense.**
A lambda inside a predicate capturing a constified variable, a predicate on
a lambda naming its own capture, a capture crossing a contract boundary
(`LambdaCapture::isCapturedAcrossContract`), `NCCK_Contract` as a fourth
kind of non-const capture -- each is a case where the ordinary rule gives
the wrong answer and the contract rule has to override it.  The
capture-depth, capture-scope and capture-note-location tests are pinning
diagnostics that were wrong in observable ways before.

`SemaCoroutine.cpp` carries [dcl.fct.def.coroutine]/5: an odr-use of a
non-reference parameter in a coroutine's postcondition is ill-formed.
Neither compiler enforced it until this branch did.

## Compile gap
Needs `1060-p2900-base-sema-core` for the contract-scope stack
(`PushContractScope`, `getCurrentContractEntry`, `isContractAssertionContext`)
and for `ResultNameDecl` attachment; it precedes this commit.

**No stubs are owed forward.**  Everything here is a rule applied on an
existing path -- `BuildDeclRefExpr`, `CheckForModifiableLvalue`,
`tryCaptureVariable` -- rather than a new entry point somebody else calls.

**Behaviour trap, and it is the important one for this commit:** without
it, every path still compiles and every predicate still evaluates.  What is
lost is silent -- a predicate may assign to the variable it is testing, a
lambda in a predicate captures by the ordinary rules, and the coroutine
parameter rule is not applied.  The failure mode of omitting this commit is
a program that builds and runs and quietly means something else, which is
why its 24 test files are nearly all `-verify` tests asserting that
specific things are *rejected*.

## Contents
- clang/include/clang/AST/LambdaCapture.h : *
- clang/include/clang/Sema/Scope.h : *
- clang/include/clang/Sema/ScopeInfo.h : *
- clang/include/clang/Sema/Sema.h : #4, #5, #6, #7, #8, #9
- clang/lib/Sema/Scope.cpp : *
- clang/lib/Sema/SemaCoroutine.cpp : *
- clang/lib/Sema/SemaExpr.cpp : *
- clang/lib/Sema/SemaExprCXX.cpp : *
- clang/lib/Sema/SemaLambda.cpp : *
- clang/lib/Sema/SemaLookup.cpp : *
- clang/test/Contracts/Sema/contract-predicate-constify-in-lambda.cpp : *
- clang/test/Contracts/Sema/contract-predicate-constify-storage.cpp : *
- clang/test/Contracts/Sema/deducing-this-no-this-in-predicate.cpp : *
- clang/test/Contracts/Sema/deducing-this-postcondition-param.cpp : *
- clang/test/Contracts/Sema/expr.prim.closure.cpp : *
- clang/test/Contracts/Sema/postcondition-discarded-operand-dependent.cpp : *
- clang/test/Contracts/Sema/postcondition-lambda-param.cpp : *
- clang/test/Contracts/Sema/postcondition-unevaluated-operand.cpp : *
- clang/test/Contracts/Sema/pr126897.cpp : *
- clang/test/Contracts/constification.cpp : *
- clang/test/Contracts/contract-capture-note-location.cpp : *
- clang/test/Contracts/contract-lambda-capture-constify.cpp : *
- clang/test/Contracts/coroutine-compat.cpp : *
- clang/test/Contracts/coroutine-postcondition-param.cpp : *
- clang/test/Contracts/lambda-capture-constify-generic-function.cpp : *
- clang/test/Contracts/lambda-capture-in-contract-error.cpp : *
- clang/test/Contracts/lambda-capture-scope-diagnostics.cpp : *
- clang/test/Contracts/lambda-contract-diagnostics.cpp : *
- clang/test/Contracts/nested-lambda-contract-capture-depth.cpp : *
- clang/test/Contracts/postcondition-param-and-capture-diagnostics.cpp : *
- clang/test/Contracts/postcondition-param-lambda.cpp : *
- clang/test/Contracts/result-name-shadows-template-param.cpp : *
- clang/test/Contracts/scope-rules.cpp : *
- clang/test/Contracts/this-in-out-of-line-member-contract.cpp : *
- clang/test/Contracts/unused-param-in-contract.cpp : *
