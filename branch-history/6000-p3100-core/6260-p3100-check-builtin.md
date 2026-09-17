---
id: 6260-p3100-check-builtin
subject: '[clang][contracts] implicit check: builtin precondition'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `builtin` check -- a builtin called with an
argument its definition leaves undefined, such as `__builtin_clz(0)`.

Routing-only, one file.  `noexcept_observe` runs the handler and continues,
since the builtin has a machine result to return.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-builtin-route-observe.cpp : *
