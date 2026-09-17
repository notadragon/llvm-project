---
id: 6140-p3100-check-returns-nonnull-attribute
subject: '[clang][contracts] implicit check: returns_nonnull attribute'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `returns-nonnull-attribute` check -- returning
null from a function declared `__attribute__((returns_nonnull))`.

Routing-only, and the mirror of
`6130-p3100-check-nonnull-attribute`: same shape, opposite side of the
call, independently configurable because the sanitizer names them
separately.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-returns-nonnull-attribute-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-returns-nonnull-attribute-route-observe.cpp : *
