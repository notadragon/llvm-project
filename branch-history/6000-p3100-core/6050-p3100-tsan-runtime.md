---
id: 6050-p3100-tsan-runtime
subject: '[clang][contracts] route TSan reports to the contract handler'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
The TSan side of P3100 routing: `tsan_rtl_report.cpp` consults the wire
byte and reports a data race through the contract-violation handler.

The smallest of the four runtimes, because TSan has essentially one report
shape.  The interesting property is not in the code: a TSan report is
produced from a *different thread's* perspective than the one that
triggered it, and the handler is called on the reporting thread, so a
handler that is not itself thread-safe is a program bug rather than a
routing one.  Nothing here can enforce that.

## Compile gap
Needs `6000-p3100-core`; it precedes this commit.

None outward.

**Behaviour trap:** the same unread-wire-byte case as the other runtimes.

## Contents

- clang/lib/CodeGen/CodeGenModule.cpp : @emitTsanContractSemanticDescriptor
- clang/test/Contracts/Runnable/p3100-tsan-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-tsan-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-tsan-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-tsan-route-quick.cpp : *
- clang/test/Contracts/p3100-tsan-sanitize-semantic.cpp : *
- compiler-rt/lib/tsan/rtl/tsan_rtl_report.cpp : *
