---
id: 6210-p3100-check-float-cast-overflow
subject: '[clang][contracts] implicit check: float cast overflow'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for UBSan's `float-cast-overflow` check -- converting a
floating-point value to an integer type that cannot represent it.

Routing-only in this commit, but with a wrinkle worth naming: the
*predicate* it depends on, `EmitFloatCastInRangePredicate`, is in
`6000-p3100-core` rather than here, because the UBSan check and the P3100
implicit guard share one definition of "in range".  Factoring it there was
deliberate -- two definitions of that predicate would be two chances to
disagree about the boundary case.

Under `noexcept_observe` the handler runs and the program continues: a
float-to-integer conversion has a machine result to carry on with.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6030-p3100-ubsan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-float-cast-overflow-route-observe.cpp : *
- clang/test/Contracts/p3100-fpint-enforce.cpp : *
- clang/test/Contracts/p3100-fpint-enforce.json : *
- clang/test/Contracts/p3100-fpint-eval.cpp : *
- clang/test/Contracts/p3100-fpint-ignore.json : *
- clang/test/Contracts/p3100-fpint-quick.cpp : *
- clang/test/Contracts/p3100-fpint-quick.json : *
- clang/test/Contracts/p3100-fpint-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-fpint-throw.cpp : *
- clang/test/Contracts/p3100-fpint-types.cpp : *
- clang/test/Contracts/p3100-fpint.cpp : *
- clang/test/Contracts/p3100-fpint.json : *
