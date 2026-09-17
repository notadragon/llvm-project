---
id: 6150-p3100-check-pointer-overflow
subject: '[clang][contracts] implicit check: pointer overflow'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
P3100 coverage for the three pointer-arithmetic checks that share a
band: UBSan's `pointer-overflow`, and ASan's `pointer-compare` and
`pointer-subtract`.

They are one commit because they are one question -- a pointer computation
whose operands do not belong to the same object -- asked at three points
the standard treats separately ([expr.add] for the overflow form,
[expr.rel] for comparison, [expr.add] again for subtraction).

**The detail worth keeping is that the two ASan checks are governed by
their own wire bytes, independent of the address routing scope.**  Turning
address routing on or off does not change what they do.  A reader who
assumes one ASan wire byte covers the whole sanitizer will misread the
configuration, and these are the checks where that assumption fails.

Under `-fcontracts-p4298` and no `-fsanitize-recover=`, all three resolve
to `noexcept_enforce`: the handler runs and the program terminates.  The
`-observe` and `-quick` variants of the tests pin the other two reactions.

## Compile gap
Needs `6000-p3100-core` for the routing framework and
``6040-p3100-asan-runtime`` for the report path; both precede it.

Nothing to stub -- this commit is coverage, not mechanism.

**Behaviour trap:** the routed and unrouted forms of this check are
distinguishable only by *where the text appears*.  A test that merely
asserts the program died passes with routing broken, which is why these
check the handler's own markers.

## Contents

- clang/test/Contracts/Runnable/p3100-pointer-compare-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-compare-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-overflow-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-overflow-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-overflow-route-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-subtract-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-pointer-subtract-route-observe.cpp : *
