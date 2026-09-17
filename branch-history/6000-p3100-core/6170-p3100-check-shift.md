---
id: 6170-p3100-check-shift
subject: '[clang][contracts] implicit check: shift'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `shift-exponent` check -- a shift by a negative
amount or by at least the operand's width.

Routing-only.  Under `noexcept_observe` the handler runs (kind 7, semantic
6) and the program continues, because a shift has a defined machine result
to carry on with even where the language does not define one.

That is the distinction this commit exists to pin, and it is the axis the
whole per-check split is organised around: whether the check has a value to
fall back to determines whether `observe` can continue at all.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-shift-exponent-route-observe.cpp : *
- clang/test/Contracts/p3100-shift-oob-enforce.cpp : *
- clang/test/Contracts/p3100-shift-oob-enforce.json : *
- clang/test/Contracts/p3100-shift-oob-eval.cpp : *
- clang/test/Contracts/p3100-shift-oob-ignore.json : *
- clang/test/Contracts/p3100-shift-oob-quick.cpp : *
- clang/test/Contracts/p3100-shift-oob-quick.json : *
- clang/test/Contracts/p3100-shift-oob-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-shift-oob-throw.cpp : *
- clang/test/Contracts/p3100-shift-oob-types.cpp : *
- clang/test/Contracts/p3100-shift-oob.cpp : *
- clang/test/Contracts/p3100-shift-oob.json : *
