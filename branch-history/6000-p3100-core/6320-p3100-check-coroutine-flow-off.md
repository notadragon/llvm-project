---
id: 6320-p3100-check-coroutine-flow-off
subject: '[clang][contracts] implicit check: flowing off a coroutine'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Control flowing off the end of a coroutine whose promise type has no usable
`return_void` -- `stmt.return.coroutine.flow.off` -- as an implicit
contract assertion.

**Deliberately not shared with `6230-p3100-check-flow-off`**, and the two
differences are both structural:

* The fall-off point is **inside the coroutine's try block**, so a throwing
  `enforce` or `observe` is caught by `promise.unhandled_exception()`
  rather than leaving the function.  That is the coroutine's own contract
  with its promise type and this check must not bypass it.
* There is **no return value to substitute**.  The coroutine's return
  object already exists and was handed to the caller at the first suspend,
  so a continuing semantic emits no return branch -- control simply falls
  through to the final suspend.

`assume` and `ignore` emit nothing, as everywhere else in the band.

## Compile gap
Needs `6000-p3100-core` for the reaction path; it precedes this commit.
`CGCoroutine.cpp` is the emission site.

Note that `0100-upstream-bugfixes` also touches `CGCoroutine.cpp`, for an
unrelated upstream defect in `ParamReferenceReplacerRAII`.  Different hunk,
no interaction.

None outward.

**Behaviour trap:** if this reused `EmitImplicitFlowOffReaction`, the
continuing semantics would emit a return branch into a function that has no
return value -- malformed IR in the best case and a wrong value handed to
the promise in the worst.  The duplication is the guard against that.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitCoroutineFlowOffReaction
- clang/lib/CodeGen/CGCoroutine.cpp : #1, #2
- clang/test/Contracts/p3100-coro-flow-off-noexcept.cpp : *
- clang/test/Contracts/p3100-coro-flow-off-noexcept.json : *
- clang/test/Contracts/p3100-coro-flow-off-quick.cpp : *
- clang/test/Contracts/p3100-coro-flow-off-quick.json : *
- clang/test/Contracts/p3100-coro-flow-off-throw.cpp : *
- clang/test/Contracts/p3100-coro-flow-off-throw.json : *
- clang/test/Contracts/p3100-coro-flow-off.cpp : *
- clang/test/Contracts/p3100-coro-flow-off.json : *
