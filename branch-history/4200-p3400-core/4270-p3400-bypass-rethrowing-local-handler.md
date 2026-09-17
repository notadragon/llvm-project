---
id: 4270-p3400-bypass-rethrowing-local-handler
subject: '[clang][contracts] bypass the checkpoint for an always-rethrowing local handler'
depends: [4200-p3400-core]
regenerates: []
fixes: []
---

## Rationale
A local violation handler that answers an `evaluation_exception` by
rethrowing it should see a predicate that threw.  Getting the exception to
it means catching it first -- which changes nothing a conforming program
can observe only if the handler really does always rethrow.  This commit
proves that, per label, and catches only when the proof succeeds.

**It is a static analysis in CodeGen, and that is unusual enough to
justify.**  The alternative is to catch unconditionally, which inserts a
landing pad and a rethrow around every labelled contract whose predicate
might throw, whether or not the handler wants it; or never to catch, which
makes the facet unreachable for the case it was designed for.
`contractLocalHandlerAlwaysRethrows` walks the handler's body under the
assumption that the exception is in flight and reports whether every path
out of it rethrows.

Three things worth a reviewer's attention:

**`RO_Returned` and `RO_Fail` are deliberately distinct**, and the comment
explains why: for the handler itself, returning is a failure -- it did not
rethrow.  For a function the handler *called*, returning means the call
completed without doing anything observable, so the walk continues past it.
Collapsing the two makes every delegating handler unprovable.

**The walk is depth-limited to 8** and the limit doubles as the termination
guard for mutual recursion.  A handler that delegates further than that is
not a shape worth proving.

**`assertionKindValue` duplicates a mapping the ABI encodes in a name.**
The entry point's *name* carries the assertion kind rather than a number,
so unlike the other seeded properties this one needs an explicit table --
and it follows `GetCxaEntryPointName`'s own post-capture handling
deliberately, so the two cannot drift.  That is the "one fact, two copies"
shape, with the second copy unavoidable and the coupling written down.

`-fno-contract-bypass-rethrowing-local-handler` turns the whole thing off.
It defaults on, and its help text states the invariant that makes the
optimisation safe: a predicate that is merely false reaches the handler
either way.

## Compile gap
Needs `4260-p3400-facet-local-violation-handler` for the handler facet
itself and `1100-p2900-base-codegen` for `BuildContractCatchAllTry`; both
precede it.

None outward.

**Behaviour trap, and it is subtle in both directions.**  With the analysis
absent, a rethrowing local handler never sees a throwing predicate -- the
exception propagates past it.  With the analysis *wrong* in the permissive
direction, a handler that does not always rethrow has its exception
swallowed, which is an observable change to a conforming program.  That
asymmetry is why the analysis fails closed: anything it cannot prove is
`RO_Fail`, and `RO_Fail` means do not catch.

## Contents

- clang/include/clang/Options/Options.td : @contract_bypass_rethro*
- clang/lib/CodeGen/CGContracts.cpp : @namespace, @AValKind, @AVal, @avUnknown, @RethrowOutcome, @RethrowMaxDepth, @assertionKindValue, @RethrowAnalysis, /AVal eval\(const Expr \*E\)/, @accessorValue, @callOutcome, @eval, @walkStmt, @contractLocalHandlerAlwaysRethrows
- clang/test/Contracts/Runnable/p3400-bypass-rethrowing-local-handler-1.cpp : *
- clang/test/Contracts/Runnable/p3400-bypass-rethrowing-local-handler-2.cpp : *
- clang/test/Contracts/Runnable/p3400-bypass-rethrowing-local-handler-3.cpp : *
- clang/test/Contracts/p3400-bypass-rethrowing-local-handler-codegen.cpp : *
- clang/test/Contracts/p3400-bypass-rethrowing-local-handler-disable-codegen.cpp : *
- clang/test/Contracts/p3400-bypass-rethrowing-local-handler-negative-codegen.cpp : *
- clang/test/Contracts/p3400-bypass-rethrowing-local-handler-noexcept-codegen.cpp : *
- clang/test/Contracts/p3400-bypass-rethrowing-local-handler-semantics-codegen.cpp : *
