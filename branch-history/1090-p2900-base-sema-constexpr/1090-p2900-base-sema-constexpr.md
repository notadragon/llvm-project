---
id: 1090-p2900-base-sema-constexpr
subject: '[clang][contracts] evaluate contracts during constant evaluation'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
Evaluating a contract during constant evaluation.

`EvaluateContract` is the whole of it in one function: resolve the
contract's constant-evaluation semantic, evaluate the predicate, and on
failure either warn and continue (`observe`) or diagnose and stop
(`enforce`).  Around it sit the three places the evaluator has to call it
from -- `EvaluatePreContracts` and `EvaluatePostContracts` bracketing
`HandleFunctionCall`, and a `Stmt::ContractStmtClass` case in
`EvaluateStmt` for a `contract_assert` in a statement position.

Three things worth a reviewer's attention:

**Observe must leave the expression constant.**  A failed `observe` emits
`warn_constexpr_contract_failure` through `Info.report` and returns
success, because [basic.contract.eval] does not make an observed violation
ill-formed.  A failed `enforce` goes through `CCEDiag`, which is what makes
the expression non-constant.  Getting that pair the wrong way round turns
either a diagnostic into a hard error or a hard error into silence, and
`contract-constexpr-multi-observe.cpp` and
`contract-constexpr-observe-cap.cpp` pin both the behaviour and the cap on
how many observes one evaluation reports.

**Reading a result name needs a hole in `findCompleteObject`.**  A
postcondition names the return value of a call that, at constant-evaluation
time, has no object to point at -- so `ResultNameDecl` is resolved against
the call frame's temporary for it, and `EvalInfo` grows a `ResultSlot` /
`ResultValue` pair to carry it.  The `FIXME` there is honest: the
validation on that path is thinner than the rest.

`HandleFunctionCall` is shared with `4000-p3098`, which opens a capture
scope inside it.  The base sequencing here is the precondition call and the
contract-result return; P3098's capture binding and teardown sit between
them and are declared as cuts of the two segments concerned, so each
commit carries only its own lines.  The teardown was given a statement of
its own for exactly that reason -- written as
`return ContractsOK && CaptureScope.destroy();` the two features share a
line and no cut can divide them.

`ByteCode/State.h` gains the `EvaluateContracts` flag that lets an
evaluation opt out entirely -- used where evaluating a predicate would
change the answer to the question being asked, such as rechecking whether
an initializer is constant.

## Compile gap
Needs `1020-p2900-base-ast` for `ContractStmt` and `ResultNameDecl`,
`1060-p2900-base-sema-core` for the semantic resolution `ensureCESemantic`
performs, and `1000-p2900-base-basic` for the semantic enumeration; all
three precede it.

`EvaluateContract` names `CES::NoexceptObserve`, which is **P4298's**, five
commits later at `4800-p4298`.  Nothing needs stubbing -- the enumerator
itself is `1000-p2900-base-basic`'s, since the base enumeration carries
every semantic and the gating is applied elsewhere -- but the comment
explaining that the noexcept variants are indistinguishable from their
plain counterparts during constant evaluation describes behaviour that
cannot be exercised until `4800-p4298` lands.

**Behaviour trap:** omitting this commit costs no diagnostic at runtime and
produces no build failure.  A contract in a `constexpr` function simply is
not evaluated at compile time, so a precondition that a constant expression
violates is accepted silently and then checked -- correctly -- only when
the same function runs.

## Contents
- clang/lib/AST/ByteCode/State.h : *
- clang/lib/AST/ExprConstant.cpp : @namespace, @public, @evaluateVarDeclInit, @findCompleteObject, @CheckLocalVariableDeclaration, @EvaluateStmt, @EvaluatePreContracts, @EvaluatePostContracts, @HandleFunctionCall, #13:0, #16:1, @EvaluateAsInitializer, @EvaluateLValue, @VisitDeclRefExpr, @VisitVarDecl
- clang/test/Contracts/Sema/expr.const.cpp : *
- clang/test/Contracts/constexpr.cpp : *
- clang/test/Contracts/contract-constexpr-multi-observe.cpp : *
- clang/test/Contracts/contract-constexpr-observe-cap.cpp : *
- clang/test/Contracts/contract-constexpr-repeat-call.cpp : *
- clang/test/Contracts/contract-constexpr-side-effect.cpp : *
- clang/test/Contracts/contract-postcondition-constexpr.cpp : *
