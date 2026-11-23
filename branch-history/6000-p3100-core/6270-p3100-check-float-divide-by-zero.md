---
id: 6270-p3100-check-float-divide-by-zero
subject: '[clang][contracts][P3100] implicit check: float divide by zero'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `float-divide-by-zero` check, and the native
implicit check beside it.

**The contrast with `6180-p3100-check-integer-divide` is why it is
separate**: floating-point division by zero has a defined IEEE result,
so `noexcept_observe` runs the handler and the program genuinely continues,
where the integer form has nothing to continue with.  Same-looking check,
opposite continuation behaviour, separately configurable.

The native check beside it is `ub:expr.mul.div.by.zero.fp.div`: [expr.mul]
makes a division by zero undefined whatever the type of its operands, so a
scalar floating-point division (and `/=`) whose divisor compares
ordered-equal to zero -- `-0.0` does, a NaN does not -- runs the reaction
and yields 0 when execution continues.  The guard is in `EmitDiv`
(`6000-p3100-core`'s `CGExprScalar.cpp`); where it applies it replaces the
sanitizer's test.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-float-divide-by-zero-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-float-divide-by-zero-throw-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-float-divide-by-zero-throw-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-fdiv-throw.cpp : *
- clang/test/Contracts/p3100-fdiv-constexpr.cpp : *
- clang/test/Contracts/p3100-fdiv-int-unaffected.cpp : *
