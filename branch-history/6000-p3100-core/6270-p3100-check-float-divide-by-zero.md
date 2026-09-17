---
id: 6270-p3100-check-float-divide-by-zero
subject: '[clang][contracts] implicit check: float divide by zero'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `float-divide-by-zero` check.

Routing-only.  **The contrast with `6180-p3100-check-integer-divide` is why
it is separate**: floating-point division by zero has a defined IEEE result,
so `noexcept_observe` runs the handler and the program genuinely continues,
where the integer form has nothing to continue with.  Same-looking check,
opposite continuation behaviour, separately configurable.

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
