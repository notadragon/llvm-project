---
id: 6040-p3100-asan-runtime
subject: '[clang][contracts] route ASan reports to the contract handler'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
The ASan side of P3100 routing: `asan_report.cpp` consults the wire byte
and reports through the contract-violation handler.

ASan differs from UBSan in a way worth knowing: its reports describe a
memory state -- a shadow byte, an allocation, a stack trace -- rather than
an expression, so the populator has more to render and the routed report is
correspondingly larger.  `CodeGenModule.h` grows a declaration here because
ASan's wire symbols are emitted from more than one place.

Sixteen files, of which thirteen are tests: the error classes ASan can
report are numerous and each needs to be shown routing correctly, since
they go through different report paths inside the runtime.

## Compile gap
Needs `6000-p3100-core`; it precedes this commit.

Note that **libasan links libubsan**, which is what made the duplicated
`extern "C"` prototype in the pre-core arrangement a real hazard rather
than a stylistic one.  With the shared header from `6000-p3100-core` in
place the two runtimes agree by construction, and this commit and
`6030-p3100-ubsan-runtime` may land in either order.

**Behaviour trap:** the same one as UBSan's -- an unread wire byte is
indistinguishable from routing being off.

## Contents

- clang/lib/CodeGen/CodeGenModule.cpp : #0, @emitAsanContractSemanticDescriptor
- clang/lib/CodeGen/CodeGenModule.h : #3
- clang/test/Contracts/Runnable/p3100-asan-guardrail-optout.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-guardrail-report-callback-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-guardrail-report-callback.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-report-default.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-report-lazy.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-observe-multi.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-observe-repeat.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-asan-route-quick.cpp : *
- clang/test/Contracts/p3100-asan-descriptor.cpp : *
- compiler-rt/lib/asan/asan_report.cpp : *
