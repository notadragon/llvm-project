---
id: 6300-p3100-check-pure-virtual
subject: '[clang][contracts] implicit check: pure virtual call'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
A call that dispatches to a pure virtual function --
`ub:class.abstract.pure.virtual` -- as an implicit contract assertion.

**This check has no call site, and that is what makes it interesting.**  A
pure-virtual dispatch is an ordinary indirect call; only the runtime
object's current vtable knows the slot is pure, so there is nowhere in the
caller to emit a guard.  The semantic is instead resolved where the
**vtable** is emitted, at the method's declaring base class, and the
vtable slot's default value changes from `__cxa_pure_virtual` to a chosen
terminus.

The consequence is a configuration granularity unlike every other check in
this band: P3595's per-file, per-line and per-namespace matching selects
the terminus **per class**, not per call.  A reviewer expecting per-call
configuration will find it absent, and the comment in `CGContracts.cpp`
explains why it cannot exist.

The vtable slot stays a plain function pointer throughout; nothing about
the ABI changes.

## Compile gap
Needs `6000-p3100-core` for semantic resolution and `2400-p3595` for the
configuration that selects the terminus; both precede it.

`CGVTables.cpp` is the vtable-emission side and is the only place outside
`CGContracts.cpp` this commit touches.

None outward.

**Behaviour trap:** with this commit absent the slot holds
`__cxa_pure_virtual` and a pure-virtual call terminates, which is the
pre-existing and perfectly reasonable behaviour -- so the absence looks
like a working program.  What is lost is the configurability, silently.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @StringRef
- clang/lib/CodeGen/CGVTables.cpp : *
- clang/test/Contracts/Runnable/p3100-function-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-function-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-function-route-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-enforce.json : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-noexcept-observe.json : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-observe.json : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-quick.json : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-throw-noexcept.cpp : *
- clang/test/Contracts/Runnable/p3100-pure-virtual-throw.cpp : *
- clang/test/Contracts/p3100-function-sanitize-semantic.cpp : *
