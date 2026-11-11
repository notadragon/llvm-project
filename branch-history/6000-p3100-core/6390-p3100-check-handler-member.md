---
id: 6390-p3100-check-handler-member
subject: '[clang][contracts][P3100] implicit check: a member in a cdtor function-try-block handler'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Referring to a non-static member or base of an object in a
function-try-block handler of its constructor or destructor --
`ub:except.handle.handler.ctor.dtor` -- as an implicit contract assertion
that is found lexically and violated unconditionally: by then every member
and base has been destroyed, or was never initialized.

`ExitCXXTryStmt` marks the emission of such a handler
(`P3100InCdtorHandler`), and a data member access (`EmitMemberExpr`) or a
member function call, named or an operator
(`EmitCXXMemberOrOperatorMemberCallExpr`), whose object is `*this` written
directly gets the violation first.  A lambda's captured `this` is in
another function, and a static member or another object is not `*this`.
The check cannot tell which members were initialized before the throw, so
it reports the parent group, as GCC does (DECISIONS.md E75).

This commit also adds the two pieces the lexical checks share:
`EmitImplicitUnconditionalCheck` (the reaction of an assertion violated
wherever it is reached; nothing defined replaces the operation, so
`observe` continues into it) and `isP3100ThisObject`.  All seven
semantics; constant evaluation is untouched.

## Compile gap
Needs `resolveImplicitSemantic` from `6000-p3100-core`, which precedes it.
The data member hook is in `CGExpr.cpp`, which
`6110-p3100-check-null-and-alignment` claims whole, and the flag and the
declarations share earlier hunks of `CodeGenFunction.h`.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitUnconditionalCheck, @isP3100ThisObject, @EmitImplicitHandlerMemberCheck
- clang/lib/CodeGen/CGException.cpp : #0
- clang/lib/CodeGen/CGExprCXX.cpp : #2:0
- clang/test/Contracts/Runnable/p3100-handler-member-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-operator.cpp : *
- clang/test/Contracts/Runnable/p3100-member-operator-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-handler-member-throw.cpp : *
- clang/test/Contracts/p3100-handler-member-constexpr.cpp : *
