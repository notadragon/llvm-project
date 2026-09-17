---
id: 6110-p3100-check-null-and-alignment
subject: '[clang][contracts] implicit checks: null dereference and misalignment'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Dereferencing a null pointer (`ub:expr.unary.dereference.nullptr`) and
accessing through a misaligned one, as implicit contract assertions.

**Two checks in one commit because they are one code path.**  Both guards
are emitted at the same place in `CGExpr.cpp` -- wherever a pointer is
about to be accessed through -- and both answer the same structural
question about the access.  Splitting them would mean two commits editing
the same twenty call sites.

The shape of each guard is worth reading once, because the rest of the
implicit-check band follows it:

* **`assume` and `ignore` return `false` and emit nothing**, leaving the
  raw access byte-identical to a build without P3100.  That is not an
  optimisation; it is the definition of those semantics.
* **The reaction is a void statement, not a value substitution.**  `*p` is
  an lvalue and there is no defined substitute for it, so unlike
  `6230-p3100-check-flow-off` -- which can supply a return value -- there
  is nothing to continue *with*.  Under `observe` the handler runs and then
  execution proceeds into the real, still-null dereference.

That last point is the one to be sure of before reading the tests:
`observe` here reports and then does the undefined thing anyway.  It buys a
diagnostic, not safety.

39 files, the largest per-check set in the band, because the access shapes
that reach these guards -- member access, array subscript, reference
binding, a `this` that is null, an access through a cast -- each arrive at
`CGExpr.cpp` by a different route.

## Compile gap
Needs `6000-p3100-core` for `resolveImplicitContractSemantic` and
`emitImplicitGuardReaction`; it precedes this commit.

`CodeGenFunction.h`'s two-line change is the declarations.

None outward.

**Behaviour trap:** both guards return a bool saying whether they emitted
anything, and the caller uses it to decide whether to also emit the
sanitizer's own check.  Ignoring the return value gives two reports for one
access under `-fsanitize=null`, which looks like a runtime bug and is a
CodeGen one.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitNullDerefGuard, @EmitImplicitMisalignedGuard
- clang/lib/CodeGen/CGExpr.cpp : *
- clang/lib/CodeGen/CodeGenFunction.h : #5, #6
- clang/test/Contracts/Runnable/p3100-align-aligned.cpp : *
- clang/test/Contracts/Runnable/p3100-align-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-align-enforce.json : *
- clang/test/Contracts/Runnable/p3100-align-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-align-ignore.json : *
- clang/test/Contracts/Runnable/p3100-align-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-align-noexcept-observe.json : *
- clang/test/Contracts/Runnable/p3100-align-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-align-observe.json : *
- clang/test/Contracts/Runnable/p3100-align-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-align-quick.json : *
- clang/test/Contracts/Runnable/p3100-align-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-report-ondemand.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-alignment-route-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-null-route-observe.cpp : *
- clang/test/Contracts/p3100-alignment-sanitize-semantic.cpp : *
- clang/test/Contracts/p3100-null-deref-addr-of.cpp : *
- clang/test/Contracts/p3100-null-deref-assume.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-enforce.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-enforce.json : *
- clang/test/Contracts/p3100-null-deref-noexcept-observe.cpp : *
- clang/test/Contracts/p3100-null-deref-noexcept-observe.json : *
- clang/test/Contracts/p3100-null-deref-observe.json : *
- clang/test/Contracts/p3100-null-deref-quick.cpp : *
- clang/test/Contracts/p3100-null-deref-quick.json : *
- clang/test/Contracts/p3100-null-deref-resolution.cpp : *
- clang/test/Contracts/p3100-null-deref-resolution.json : *
- clang/test/Contracts/p3100-null-deref-throw-cleanup.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-fntryblock.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-noexcept.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-null-deref-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-null-ref-and-call.cpp : *
- clang/test/Contracts/p3100-null-ref-and-call.json : *
