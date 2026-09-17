---
id: 6240-p3100-check-unreachable
subject: '[clang][contracts] implicit check: unreachable'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `unreachable` check -- reaching a
`__builtin_unreachable()`.

Routing-only, and the smallest entry in the band: one file.  Like
`6180-p3100-check-integer-divide` there is nowhere to continue to, so
`noexcept_observe` runs the handler once and then the violation terminates
or faults.

It is its own commit rather than being folded into a neighbour because the
per-check split is what lets a check be added or dropped as the work
continues, and a one-file check is exactly the kind that would otherwise be
lost inside a larger one.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-unreachable-route-observe.cpp : *
