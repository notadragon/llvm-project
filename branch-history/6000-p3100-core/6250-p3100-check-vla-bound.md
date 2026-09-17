---
id: 6250-p3100-check-vla-bound
subject: '[clang][contracts] implicit check: VLA bound'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `vla-bound` check -- a variable-length array
whose bound is not strictly positive.

Routing-only, one file.  Under `noexcept_observe` the handler runs and the
program continues.  The test passes `-Wno-vla-cxx-extension`, since a VLA
is not standard C++ and the warning would otherwise be the test's output.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-vla-bound-route-observe.cpp : *
