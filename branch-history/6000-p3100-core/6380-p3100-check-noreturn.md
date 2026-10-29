---
id: 6380-p3100-check-noreturn
subject: '[clang][contracts][P3100] implicit check: a [[noreturn]] function returns'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
A function declared `[[noreturn]]` (or noreturn by any other means) that
returns to its caller -- `ub:dcl.attr.noreturn.eventually.returns` -- as
an implicit contract assertion made in the callee, so that a call through
a function pointer is covered as well as a direct call.

Every `return` and the fall-off branch to the return block through the
cleanups, so the check is made there, in `FinishFunction`, after the
function's locals are destroyed; an exception or a `longjmp` leaving the
function never reaches it.  Where the epilogue evaluates postconditions,
`EmitPostContracts` makes the check after them instead
(`P3100NoreturnCheckAfterPosts`), the order GCC uses.  A return block
nothing branches to is not checked.

The caller was compiled assuming the call does not return, so there is no
valid continuation: `observe` calls the handler and then aborts, as the
pure-virtual termini of `6300-p3100-check-pure-virtual` do, and `ignore`
keeps today's behaviour.  Constant evaluation is untouched.

## Compile gap
Needs `resolveImplicitSemantic` from `6000-p3100-core`, and the
postcondition epilogue of `1100-p2900-base-codegen` (cut of
`CGCall.cpp#3`); both precede it.  The flag and the declaration share
earlier hunks of `CodeGenFunction.h`.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitNoreturnCheck
- clang/lib/CodeGen/CGCall.cpp : #3:3
- clang/lib/CodeGen/CodeGenFunction.cpp : #2
- clang/test/Contracts/Runnable/p3100-noreturn-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-post.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-noreturn-throw.cpp : *
- clang/test/Contracts/p3100-noreturn-assume.cpp : *
- clang/test/Contracts/p3100-noreturn-codegen.cpp : *
- clang/test/Contracts/p3100-noreturn-constexpr.cpp : *
