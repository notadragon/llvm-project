---
id: 6310-p3100-check-enum-cast
subject: '[clang][contracts] implicit check: out-of-range enum cast'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Casting an integer to an enumeration type outside its `[dcl.enum]` value
range -- `expr.static.cast.enum.outside.range` -- as an implicit contract
assertion, for an enumeration with no fixed underlying type.

Routing-only in this commit: `EmitEnumCastInRangePredicate` is in
`6000-p3100-core`, alongside the float-cast predicate and for the same
reason -- the sanitizer check and the implicit guard must agree about the
boundary.

**The default is `assume`**, and the first test pins exactly that: with no
configuration, no check is emitted and the raw out-of-range result is kept.
That is a deliberate choice rather than an oversight -- this conversion is
common in code that predates scoped enumerations -- and it is the one check
in the band whose default is not to check.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-enum-cast-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-enforce.json : *
- clang/test/Contracts/Runnable/p3100-enum-cast-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-ignore.json : *
- clang/test/Contracts/Runnable/p3100-enum-cast-inrange.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-noexcept-observe.json : *
- clang/test/Contracts/Runnable/p3100-enum-cast-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-observe.json : *
- clang/test/Contracts/Runnable/p3100-enum-cast-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-cast-quick.json : *
- clang/test/Contracts/Runnable/p3100-enum-cast-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-enum-route-observe.cpp : *
- clang/test/Contracts/p3100-enum.cpp : *
