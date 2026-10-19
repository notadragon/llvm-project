---
id: 6350-p3100-check-null-member-pointer
subject: '[clang][contracts][P3100] implicit check: null pointer to member'
depends: [6000-p3100-core, 6110-p3100-check-null-and-alignment]
regenerates: []
fixes: []
---

## Rationale
Applying a null pointer to member with `.*` or `->*` --
`ub:expr.mptr.oper.member.func.null` -- as an implicit contract
assertion, with one leaf per kind of member pointer, as GCC names them:
`.data` where `EmitPointerToDataMemberBinaryExpr` adds a pointer to data
member's offset to the object address, and `.function` where
`EmitCXXMemberPointerCallExpr` calls through a pointer to member function.

The null test is the ABI's own (`EmitMemberPointerIsNotNull`): an offset
of -1 for a data member, a zero function pointer with a clear virtual bit
where the ABI keeps that bit in the adjustment.  A constant member pointer
is not checked.  Nothing defined can replace the member pointer, so the
reaction is a void statement, as for a null dereference: `observe` runs
the handler and goes on with the pointer.  Constant evaluation is
untouched; the evaluator already rejects a null member pointer.

## Compile gap
Needs `resolveImplicitSemantic` and `emitImplicitGuardReaction` from
`6000-p3100-core`; both precede it.  The data-member hook is in
`CGExpr.cpp`, which `6110-p3100-check-null-and-alignment` claims whole,
and the declaration shares an earlier hunk of `CodeGenFunction.h`, so this
commit carries the function, the call hook in `CGExprCXX.cpp` and the
tests.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitNullMemberPointerGuard
- clang/lib/CodeGen/CGExprCXX.cpp : #15
- clang/test/Contracts/Runnable/p3100-mptr-null-data-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-data-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-mptr-null-fn-throw.cpp : *
- clang/test/Contracts/p3100-mptr-null-data-constexpr.cpp : *
- clang/test/Contracts/p3100-mptr-null-fn-assume.cpp : *
- clang/test/Contracts/p3100-mptr-null-fn-constexpr.cpp : *
