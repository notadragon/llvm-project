---
id: 6400-p3100-check-base-init-mem-fun
subject: '[clang][contracts][P3100] implicit check: operations on *this before its bases are initialized'
depends: [6000-p3100-core, 6390-p3100-check-handler-member]
regenerates: []
fixes: []
---

## Rationale
Calling a member function of an object, or applying `typeid` or
`dynamic_cast` to it, before the mem-initializers for its base classes
have completed or after its bases are destroyed --
`ub:class.base.init.mem.fun` -- as an implicit contract assertion for the
direct forms on `*this`, found lexically and violated unconditionally, in
a base-class or delegating mem-initializer (marked by `EmitCtorPrologue`,
`P3100InBaseMemInit`), a constructor precondition or a destructor
postcondition (the contract whose predicate is being emitted,
`CurContract`).

The leaves follow the D4277R1 split, with GCC's names: a virtual call,
`typeid` of a polymorphic `*this` and a `dynamic_cast` of a polymorphic
`this` or `*this`, whatever its cast kind, are `.polymorphic.virtual.call`,
`.polymorphic.typeid` and `.polymorphic.dynamic.cast`; any other member
function call, named or an operator, is `.nonpolymorphic.mem.init` or
`.nonpolymorphic.contract`.  A member mem-initializer, the body, a static
member function, another object, a constructor postcondition, a destructor
precondition and a lambda's captured `this` are not checked.  All seven
semantics; constant evaluation is untouched.

## Compile gap
Needs `EmitImplicitUnconditionalCheck` and `isP3100ThisObject` from
`6390-p3100-check-handler-member`, and `resolveImplicitSemantic` from
`6000-p3100-core`; both precede it.  The `dynamic_cast` hooks are in
`CGExpr.cpp` and `CGExprScalar.cpp`, which
`6110-p3100-check-null-and-alignment` and `6000-p3100-core` claim whole,
and the flag and the declarations share earlier hunks of
`CodeGenFunction.h`.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @isP3100CdtorContract, @EmitImplicitThisOperationCheck, @EmitImplicitDynamicCastThisCheck
- clang/lib/CodeGen/CGClass.cpp : #1, #2
- clang/lib/CodeGen/CGExprCXX.cpp : #2:1, #24
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-operator.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-poly.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-base-init-mem-fun-throw.cpp : *
- clang/test/Contracts/p3100-base-init-mem-fun-constexpr.cpp : *
- clang/test/Contracts/p3100-base-init-mem-fun-split.cpp : *
