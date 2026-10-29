---
id: 1090-p2900-base-sema-constexpr
subject: '[clang][contracts][P2900] evaluate contracts during constant evaluation'
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

**What a predicate modifies is undone.**  `[basic.contract.eval]` permits
an evaluation that produces a predicate's value without its side effects,
so `EvaluateContract` evaluates the predicate for real and then undoes
everything it modified (`PredicateRollback`): the enclosing evaluation
proceeds as if the predicate had not been evaluated and means the same
under every evaluation semantic.  `findSubobject`, which every modifying
access goes through, saves the complete object the first time; restoring
happens in place, because the enclosing evaluation may be initializing a
subobject of it; locals and temporaries the predicate created are left
alone (`createLocal` reports them), and a heap allocation it deallocates
is revived.  `contract-constexpr-side-effect-rollback.cpp` has the shapes.

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

**A variable's constant destruction reports its problems too.**
`EvaluateDestruction` runs the destructor in an `EvalInfo` of its own, so
it has a `ContractProblemReport` of its own, keyed by the variable's
location: a `constexpr` variable or a class-type template argument, whose
destruction must be constant, reports them as any constant evaluation does;
for any other variable the destruction is evaluated only to learn whether
the object needs destroying at exit, so a problem just makes it
non-constant and the object is destroyed, and its destructor's assertions
evaluated, at run time.  `constexpr-dtor-contracts-constant-destruction.cpp`.

`ByteCode/State.h` gains the `EvaluateContracts` flag that lets an
evaluation opt out entirely -- used where evaluating a predicate would
change the answer to the question being asked, such as rechecking whether
an initializer is constant.  `ByteCode/EvalSettings.h` carries the
same flag into the bytecode interpreter's `InterpState`.

The bytecode interpreter (`-fexperimental-new-constant-interpreter`) gets
the same behaviour in `ByteCode/Compiler.cpp`: a `ContractStmt` compiles to
a predicate test and a `ContractFailed` op that reports as
`EvaluateContract` does, preconditions and postcondition captures are
emitted at function entry, and when a function has postconditions every
return jumps to one epilogue that checks them.  A scalar or reference result
is kept in a local the result name denotes; a class result is the object
behind the RVO pointer.  `emitCleanup` stops at the scope holding the
captures, which a return must not destroy before the postconditions read
them.

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
- clang/lib/AST/ByteCode/Compiler.cpp : *
- clang/lib/AST/ByteCode/Compiler.h : *
- clang/lib/AST/ByteCode/EvalSettings.h : *
- clang/lib/AST/ByteCode/Interp.cpp : *
- clang/lib/AST/ByteCode/Interp.h : *
- clang/lib/AST/ByteCode/InterpState.cpp : *
- clang/lib/AST/ByteCode/Opcodes.td : *
- clang/lib/AST/ByteCode/State.h : #1, #2
- clang/lib/AST/ExprConstant.cpp : @namespace, #34, #43, @findCompleteObject, @EvaluateStmt, @EvaluatePreContracts, @EvaluatePostContracts, @HandleFunctionCall, #21:0, #24:1, @EvaluateAsInitializer, @doEvaluateInitializer, @EvaluateAsMandatedConstantInitializer, @VisitDeclRefExpr, @VisitVarDecl, /ContractProblem|OldContractProblems/, /Report\.finish\(/, /MustBeConstant/, #14, #15, /InPredicate\(/
- clang/test/Contracts/Sema/expr.const.cpp : *
- clang/test/Contracts/constexpr-postcondition-violation.cpp : *
- clang/test/Contracts/p2900-constexpr-potential-postcondition.cpp : *
- clang/test/Contracts/contract-constexpr-multi-observe.cpp : *
- clang/test/Contracts/contract-constexpr-observe-cap.cpp : *
- clang/test/Contracts/contract-constexpr-repeat-call.cpp : *
- clang/test/Contracts/contract-constexpr-repeat-call-shapes.cpp : *
- clang/test/Contracts/contract-constexpr-side-effect.cpp : *
- clang/test/Contracts/contract-postcondition-constexpr.cpp : *
- clang/test/Contracts/constexpr-contract-every-problem.cpp : *
- clang/test/Contracts/constexpr-contract-array-bound.cpp : *
- clang/test/Contracts/constexpr-contract-observe-warning-once.cpp : *
- clang/test/Contracts/matrix-notional-*.cpp : *
- clang/test/Contracts/contract-constexpr-side-effect-rollback.cpp : *
- clang/test/Contracts/contract-constexpr-side-effect-nested.cpp : *
- clang/test/Contracts/contract-constexpr-side-effect-warn.cpp : *
- clang/test/Contracts/contract-nested-modifiable-tracker.cpp : *
- clang/test/Contracts/contract-assume-nesting-permutations.cpp : *
- clang/test/Contracts/contract-discardable-evaluation.cpp : *
- clang/test/Contracts/constexpr-dtor-contracts-constant-destruction.cpp : *
