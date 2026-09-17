---
id: 6280-p3100-check-implicit-conversion
subject: '[clang][contracts] implicit check: implicit conversion'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `implicit-conversion` group -- an implicit
integral conversion that changes the value, such as `300` truncated to
`signed char`.

**This is not core undefined behaviour**, and it is the only entry in the
band that is not.  The conversion is well defined; it is merely almost
always a mistake.  Routing it means the same handler that hears about UB
can also hear about this class of well-defined-but-flagged behaviour, which
is a question P3100 raises and does not settle.

**No GCC counterpart** -- this is one of the two band numbers that exist on
Clang alone.

Under `noexcept_observe` the handler runs (kind 7, semantic 6) and the
program continues with the truncated value, which is the value it would
have had anyway.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-implicit-conversion-route-observe.cpp : *
