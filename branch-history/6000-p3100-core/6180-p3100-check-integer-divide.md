---
id: 6180-p3100-check-integer-divide
subject: '[clang][contracts] implicit check: integer divide'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `integer-divide-by-zero` check.

**The contrast with `6170-p3100-check-shift` is the point.**  Division by
zero has no defined fallback -- there is no result to continue with -- so
even `noexcept_observe` runs the handler exactly once and then the
violation terminates or faults.  `observe` does not mean "continue" here;
it means "report before dying", and the test's `not %t` is what records
that.

25 files, because the operand shapes that reach this check differ (signed,
unsigned, the `INT_MIN / -1` case, constant and non-constant divisors) and
they do not share a code path in the runtime.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-integer-divide-by-zero-route-observe.cpp : *
- clang/test/Contracts/p3100-div-by-zero-enforce.cpp : *
- clang/test/Contracts/p3100-div-by-zero-enforce.json : *
- clang/test/Contracts/p3100-div-by-zero-ignore.json : *
- clang/test/Contracts/p3100-div-by-zero-observe.json : *
- clang/test/Contracts/p3100-div-by-zero-quick.cpp : *
- clang/test/Contracts/p3100-div-by-zero-quick.json : *
- clang/test/Contracts/p3100-div-by-zero-throw-cleanup.cpp : *
- clang/test/Contracts/p3100-div-by-zero-throw-fntryblock.cpp : *
- clang/test/Contracts/p3100-div-by-zero-throw-noexcept.cpp : *
- clang/test/Contracts/p3100-div-by-zero-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-div-by-zero-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-div-by-zero.cpp : *
- clang/test/Contracts/p3100-div-by-zero.json : *
- clang/test/Contracts/p3100-div-eval.cpp : *
- clang/test/Contracts/p3100-div-overflow-enforce.cpp : *
- clang/test/Contracts/p3100-div-overflow-enforce.json : *
- clang/test/Contracts/p3100-div-overflow-independent.cpp : *
- clang/test/Contracts/p3100-div-overflow-independent.json : *
- clang/test/Contracts/p3100-div-overflow.cpp : *
- clang/test/Contracts/p3100-div-overflow.json : *
- clang/test/Contracts/p3100-div-types.cpp : *
- clang/test/Contracts/p3100-mod-by-zero-enforce.cpp : *
- clang/test/Contracts/p3100-mod-by-zero-quick.cpp : *
- clang/test/Contracts/p3100-mod-by-zero-throw.cpp : *
