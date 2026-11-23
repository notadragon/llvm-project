---
id: 6340-p3100-check-pointer-arithmetic
subject: '[clang][contracts][P3100] implicit checks: pointer arithmetic'
depends: [6000-p3100-core, 6110-p3100-check-null-and-alignment, 6220-p3100-check-bounds]
regenerates: []
fixes: []
---

## Rationale
Three implicit contract assertions on pointer arithmetic, each with all
seven semantics and the unmoved pointer as the defined substitute.

`ub:expr.add.out.of.bounds.known.arithmetic`: `a + i`, `i + a` or `a - i`,
where `a` is an array of statically-known bound N decayed to a pointer to
its first element, must stay within [0, N].  It is the arithmetic leaf
beside the subscript check of `6220-p3100-check-bounds`, and like it skips
a trailing array that may be used as a flexible array member.  The guard
is in `EmitPointerArithmetic`, on the offset once it is widened to the
index type and negated for a subtraction, so a negative offset, or an
unsigned one standing for -1, compares above N.  A constant offset in range
is not guarded.

Arithmetic and subscripts applied to known-bound arithmetic are checked
against the same array at the combined position: `EmitPointerArithmetic`
records in `P3100KnownArith` the position each known-bound result
designates, keyed by the expression and valid only while the result value
matches, so `(a + i) + j`, `*(a + i + j)` and `a + i + j + k` are checked
as `a + (i + j)` would be, and a subscript `(a + k)[i]` as `a[k + i]`
(`EmitP3100NestedPosition`; an unsigned index above the bound counts as
outside, so the sum cannot wrap back into range).  Execution that continues
past a violation there gets a pointer to the first element.

`ub:expr.unary.dereference.invalid.known`: an indirection through the
past-the-end pointer is undefined in C++ even when only its address is
used.  An unaccessed subscript `&a[N]` (or a reference bound to `a[N]`,
or a trivial copy of an empty class from or to `a[N]`, which reads or
writes no object -- `BoundsAccessed`, the sanitizer's bounds check still
treating the store as accessed) is checked after the subscript check has
redirected its index, and the
operand of `*` or `->` that is known-bound arithmetic -- `*(a + N)`,
`&*(a + N)`, `(a + N)->m`, `(a + N)->f()`, `(a + N)->*pm` -- is marked
by `P3100IndirectedArith` for
`EmitPointerArithmetic` to check after the arithmetic guard, so that an
index either earlier check reported is not reported twice.

`ub:expr.add.out.of.bounds.pointer.arithmetic`: pointer arithmetic whose
result wraps around the address space, tested in `EmitCheckedInBoundsGEP`
as the overflow half of `-fsanitize=pointer-overflow` tests it, so `p + i`,
`p - i`, `p += i`, `++p`, `p--` and `p[i]` are all covered.  It is not the
routed pointer-overflow check of `6150-p3100-check-pointer-overflow`, which
stays sanitizer instrumentation; where both apply the contract replaces the
sanitizer's overflow test.  A GEP that folded to a constant is not guarded.

Constant evaluation is untouched: the evaluator already rejects arithmetic
past N and a read past the end (it accepts `&*(a + N)`, the stock bug
CLANG-712), and the wrap test is code
generation only.

## Compile gap
Needs `EmitImplicitSubstitutingGuard` and `resolveImplicitSemantic` from
`6000-p3100-core`, `getP3100KnownArrayBound` (in `CGExpr.cpp`) from
`6110-p3100-check-null-and-alignment` and `EmitImplicitArrayBoundsGuard`
from `6220-p3100-check-bounds`; all precede it.
The code is in `CGExpr.cpp` and `CGExprScalar.cpp`, which
`6110-p3100-check-null-and-alignment` and `6000-p3100-core` claim whole,
so this commit carries the two position helpers and the tests; the
`P3100IndirectedArith` flag and the `P3100KnownArith` record share earlier
hunks of `CodeGenFunction.h`.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @P3100KnownArithInfo, @EmitP3100NestedPosition
- clang/test/Contracts/Runnable/p3100-arith-bounds-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-nested.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-nested-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-nested-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-nested-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-member-call.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-split.cpp : *
- clang/test/Contracts/Runnable/p3100-arith-bounds-split-parent.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-with-subscript.cpp : *
- clang/test/Contracts/Runnable/p3100-invalid-known-address.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-incdec.cpp : *
- clang/test/Contracts/Runnable/p3100-ptr-wrap-incdec-edges.cpp : *
- clang/test/Contracts/p3100-arith-bounds-constexpr.cpp : *
- clang/test/Contracts/p3100-arith-bounds-constinit.cpp : *
- clang/test/Contracts/p3100-ptr-wrap-constexpr.cpp : *
- clang/test/Contracts/p3100-ptr-wrap-constinit.cpp : *
- clang/test/Contracts/p3100-arith-bounds-nested-const.cpp : *
- clang/test/Contracts/p3100-arith-bounds-nested-constexpr.cpp : *
- clang/test/Contracts/p3100-invalid-known-constexpr.cpp : *
- clang/test/Contracts/p3100-ptr-wrap-incdec-constexpr.cpp : *
