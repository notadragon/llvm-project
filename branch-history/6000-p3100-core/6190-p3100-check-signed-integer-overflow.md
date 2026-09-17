---
id: 6190-p3100-check-signed-integer-overflow
subject: '[clang][contracts] implicit check: signed integer overflow'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
Signed integer overflow as an **implicit contract assertion**:
`ub:expr.expr.eval.signed.integer`.

Unlike the routing-only commits in this band, this one emits a check where
no sanitizer is enabled.  `EmitImplicitSignedOverflowOp` is the guard, and
`CGExprScalar.cpp` (in `6000-p3100-core`) calls it at each arithmetic site.

**The precedence rule is the thing to read.**  The implicit assertion takes
precedence over the existing overflow-behaviour and sanitizer handling, but
only when `-fsanitize=signed-integer-overflow` is *not* on -- if it is, the
sanitizer's own routed path handles it and emitting both would report
twice.  The condition at each call site says exactly that, and it is the
pattern every implicit guard in this band follows.

## Compile gap
Needs `6000-p3100-core` for the guard-reaction path and the call sites that
invoke it; it precedes this commit.

None outward.

**Behaviour trap:** with the guard absent, signed overflow behaves as it
always has -- wrapping, trapping or being optimised away according to
`-fwrapv` and friends -- and nothing indicates that the implicit assertion
was requested and not emitted.

## Contents

- clang/lib/CodeGen/CGContracts.cpp : @EmitImplicitSignedOverflowOp
- clang/test/Contracts/Runnable/p3100-signed-integer-overflow-route-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-signed-integer-overflow-route-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-unsigned-integer-overflow-route-observe.cpp : *
- clang/test/Contracts/p3100-overflow-codegen.cpp : *
- clang/test/Contracts/p3100-overflow-codegen.json : *
- clang/test/Contracts/p3100-overflow-enforce.cpp : *
- clang/test/Contracts/p3100-overflow-enforce.json : *
- clang/test/Contracts/p3100-overflow-noexcept.cpp : *
- clang/test/Contracts/p3100-overflow-noexcept.json : *
- clang/test/Contracts/p3100-overflow-quick.cpp : *
- clang/test/Contracts/p3100-overflow-quick.json : *
- clang/test/Contracts/p3100-overflow-throw-nonnoexcept.cpp : *
- clang/test/Contracts/p3100-overflow-throw-nonnoexcept.json : *
- clang/test/Contracts/p3100-overflow.cpp : *
- clang/test/Contracts/p3100-overflow.json : *
