---
id: 6030-p3100-ubsan-runtime
subject: '[clang][contracts] route UBSan reports to the contract handler'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
The UBSan side of P3100 routing: `ubsan_diag.cpp` consults the wire byte
before reporting, and reports through the contract-violation handler when
routing is on.

UBSan is the first runtime in the band and the largest of the four, because
it is the one with the most report shapes -- its diagnostic rendering is
shared by every `-fsanitize=undefined` check, so the routing decision has
to be made once at the point where a report becomes text rather than at
each check.

`CodeGenModule.cpp`'s 124 lines are the compiler half: emitting the weak
wire symbol per check and the populator the handler calls back through.

The lazy populator is the design point.  Rendering a UBSan report allocates
and is not cheap, and the common case is a handler that never asks for it,
so the text is produced only if `report()` is called.

## Compile gap
Needs `6000-p3100-core` for the routing vocabulary and the wire encoding;
it precedes this commit.

None outward, except that `5200-p4301`'s `report()` has nothing to return
until this commit or one of its siblings lands -- that entry records the
dependency from its side.

Every UB check in `6100`-`6330` that is a UBSan check depends on this
commit as well as on the core.

**Behaviour trap:** with the compiler half present and the runtime half
absent, the wire byte is written and never read, so every check reports in
its stock form.  That is precisely the pre-routing behaviour, which is why
it is easy to miss.

## Contents

- clang/lib/CodeGen/CodeGenModule.cpp : @emitUbsanContractSemanticDescriptor
- compiler-rt/lib/ubsan/ubsan_diag.cpp : *
- compiler-rt/lib/ubsan/ubsan_diag.h : *
