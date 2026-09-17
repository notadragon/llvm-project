---
id: 6230-p3100-check-flow-off
subject: '[clang][contracts] implicit check: flowing off the end'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Control flowing off the end of a value-returning function --
`stmt.return.flow.off` -- as an implicit contract assertion.

`EmitImplicitFlowOffReaction` returns a bool, and that signature is the
design: it reports whether it emitted a non-`assume` reaction, because if
it did the caller must **not** also emit its own missing-return handling.
Emitting both produces either two reports or unreachable code after a
terminating call.

The reaction differs by semantic in a way the other checks do not need:
a terminating semantic leaves the insertion point cleared, while a
continuing one emits a *defined* return value and branches to the epilogue
-- so `observe` here does not merely report and carry on, it supplies the
value the language declined to.  `assume` returns false and leaves today's
undefined behaviour exactly as it was.

32 files, second only to `6110`, because the
return types that need a defined value (scalar, class, reference, void
specialisations of templates) each produce a different epilogue.

## Compile gap
Needs `6000-p3100-core` for the reaction path; it precedes this commit.

`6320-p3100-check-coroutine-flow-off` is the coroutine form of the same
UB and deliberately does *not* share this function -- see that entry for
why.

**Behaviour trap:** if the caller's missing-return handling is not
suppressed when this returns true, the failure is not a wrong answer but a
malformed function -- two terminators, or code after a `noreturn` call.
The IR verifier catches it; the contracts suite would not.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitFlowOffReaction
- clang/lib/CodeGen/CodeGenFunction.cpp : #8
- clang/test/Contracts/Runnable/p3100-return-route-observe.cpp : *
- clang/test/Contracts/p3100-flow-off-default.cpp : *
- clang/test/Contracts/p3100-flow-off-enforce-aggregate.cpp : *
- clang/test/Contracts/p3100-flow-off-enforce.cpp : *
- clang/test/Contracts/p3100-flow-off-enforce.json : *
- clang/test/Contracts/p3100-flow-off-fntryblock-handler.cpp : *
- clang/test/Contracts/p3100-flow-off-fntryblock-noexcept-terminate.cpp : *
- clang/test/Contracts/p3100-flow-off-fntryblock-noexcept.cpp : *
- clang/test/Contracts/p3100-flow-off-fntryblock-observe.json : *
- clang/test/Contracts/p3100-flow-off-fntryblock.cpp : *
- clang/test/Contracts/p3100-flow-off-fntryblock.json : *
- clang/test/Contracts/p3100-flow-off-ignore.cpp : *
- clang/test/Contracts/p3100-flow-off-ignore.json : *
- clang/test/Contracts/p3100-flow-off-noexcept.cpp : *
- clang/test/Contracts/p3100-flow-off-noexcept.json : *
- clang/test/Contracts/p3100-flow-off-observe.cpp : *
- clang/test/Contracts/p3100-flow-off-observe.json : *
- clang/test/Contracts/p3100-flow-off-quick.cpp : *
- clang/test/Contracts/p3100-flow-off-quick.json : *
- clang/test/Contracts/p3100-flow-off-throw-cleanup.cpp : *
- clang/test/Contracts/p3100-flow-off-throw-enforce.json : *
- clang/test/Contracts/p3100-flow-off-throw-no-return-dtor.cpp : *
- clang/test/Contracts/p3100-flow-off-throw-noexcept-enforce.cpp : *
- clang/test/Contracts/p3100-flow-off-throw-noexcept-observe.cpp : *
- clang/test/Contracts/p3100-flow-off-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-flow-off-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-flow-off-throw-observe.json : *
- clang/test/Contracts/p3100-flow-off-throw-trycatch.cpp : *
- clang/test/Contracts/p3100-flow-off-zero-return-observe.cpp : *
- clang/test/Contracts/p3100-flow-off-zero-return.cpp : *
