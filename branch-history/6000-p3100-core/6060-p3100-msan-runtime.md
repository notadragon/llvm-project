---
id: 6060-p3100-msan-runtime
subject: '[clang][contracts] route MSan reports to the contract handler'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
The MSan side of P3100 routing: `msan.cpp` consults the wire byte and
reports a use of uninitialized memory through the contract-violation
handler.

**This commit has no GCC counterpart** -- GCC has no MemorySanitizer at
all, which is why this band number exists on Clang and not there.  It is
one of the two places where the two mappings deliberately diverge rather
than mirroring each other.

## Compile gap
Needs `6000-p3100-core`; it precedes this commit.

None outward.

**Behaviour trap:** the same unread-wire-byte case.  Additionally, MSan
requires the whole program including libc++ to be instrumented, so a routed
MSan report that never fires may mean the routing is broken *or* that the
build was not fully instrumented.  The tests here build their own
dependencies for that reason.

## Contents

- clang/lib/CodeGen/CodeGenModule.cpp : @emitMsanContractSemanticDescriptor
- clang/test/Contracts/Runnable/p3100-msan-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-msan-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-msan-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-msan-route-quick.cpp : *
- clang/test/Contracts/p3100-msan-sanitize-semantic.cpp : *
- compiler-rt/lib/msan/msan.cpp : *
