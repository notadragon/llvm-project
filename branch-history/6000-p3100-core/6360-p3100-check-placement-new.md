---
id: 6360-p3100-check-placement-new
subject: '[clang][contracts][P3100] implicit checks: the pointer of a placement new'
depends: [6000-p3100-core, 6110-p3100-check-null-and-alignment]
regenerates: []
fixes: []
---

## Rationale
The pointer argument of a non-allocating placement new, which the
allocation function returns, checked before the objects are initialized
there, by two implicit contract assertions with GCC's names:

`ub:expr.new.non.allocating.null`: the pointer must not be null.  Since
DR 1748 the result of a placement new is not tested for null, so a null
pointer has an object initialized at address zero.

`ub:basic.align.object.alignment`: the pointer must be suitably aligned
for the type created -- checked against the type's alignment, not against
the alignment the address is known to have, which is smaller exactly when
it is misaligned (the type check of the new-expression used the latter and
missed `new (buf + 1) S()` for a local `buf`).  The check is the `.nostore`
leaf where the new-expression stores nothing -- default-initialization
that performs no initialization, or an empty class whatever its
initializer -- since no access check can see the address then, and the
`.store` leaf, the access check's, where its initialization stores.

Both take the place of the null and alignment assertions of the
new-expression's type check (`P3100PlacementNewChecked`) and, where they
apply, of the sanitizer's tests.  A class's own placement operator new is
not checked.  All seven semantics; execution continuing past a violation
goes on with the pointer.  Constant evaluation is untouched: the evaluator
already rejects a placement new at a null pointer.

## Compile gap
Needs `resolveImplicitSemantic` and `emitImplicitGuardReaction` from
`6000-p3100-core`, and the type check it suppresses from
`6110-p3100-check-null-and-alignment`; both precede it.  The suppression
is in `CGExpr.cpp`, which `6110-p3100-check-null-and-alignment` claims
whole, and the flag and the declaration share earlier hunks of
`CodeGenFunction.h`, so this commit carries the guards, the hooks in
`EmitCXXNewExpr` and the tests.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @placementNewStoresNothing, @EmitImplicitPlacementNewGuards
- clang/lib/CodeGen/CGExprCXX.cpp : #18, #19, #20, #21
- clang/test/Contracts/Runnable/p3100-placement-align-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-known.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-char-array.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-align-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-placement-null-throw.cpp : *
- clang/test/Contracts/p3100-placement-align-constexpr.cpp : *
- clang/test/Contracts/p3100-placement-align-split.cpp : *
- clang/test/Contracts/p3100-placement-null-constexpr.cpp : *
