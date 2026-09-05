# CLANG-1: constexpr evaluation of virtual-base conversions skips the liveness check

**Status:** Fixed here (commit `8b33919f7aea`)
**Component:** clang / constexpr
**Upstream Link:** UNKNOWN -- no search for an existing upstream report has
been attempted yet

## Bug Report

Converting a pointer or glvalue to a virtual base consults the most-derived
object's layout -- a use of the object -- but Clang's virtual-base conversion
path had no liveness check, unlike the ordinary member-access and
lvalue-to-rvalue paths. This let a constant expression form a virtual-base
member's address through deallocated or not-yet-constructed storage, which
should instead be rejected as undefined behavior in a constant expression.

**Versions.** Measured 2026-09-04 with the full `.cpp` file below,
`-std=c++26 -fsyntax-only`:

| Clang version | Result |
|---|---|
| 18.1.0 | REJECTED (but for an unrelated reason -- see below) |
| 19.1.0 | REJECTED (but for an unrelated reason -- see below) |
| 20.1.0 | REJECTED (but for an unrelated reason -- see below) |
| 21.1.0 | REJECTED (but for an unrelated reason -- see below) |
| 22.1.6 | REJECTED (but for an unrelated reason -- see below) |
| trunk | ACCEPTED (bug) |

18.1.0 through 22.1.6 reject the whole file, but not for the reason this
report is about: in those releases, a class with a virtual base (e.g.
`struct D : virtual B {}`) has no constexpr default constructor at all, so
every case fails before ever reaching the virtual-base conversion path
("non-constexpr constructor ... cannot be used in a constant expression" /
"constexpr function never produces a constant expression"). Only trunk has
gained a constexpr default constructor for such classes, and on trunk the
whole file is accepted silently -- that silent acceptance is the actual
bug. So this defect is presently observable only on trunk; it has not
"always reproduced" on a released compiler.

## Reproducer

See [`clang-01-constexpr-vbase-lifetime.cpp`](clang-01-constexpr-vbase-lifetime.cpp) in this directory.

## Our Fix

`clang/lib/AST/ExprConstant.cpp` (`HandleLValueBase`): gate the virtual-base
branch on `checkDynamicType(..., AK_DynamicCast, Polymorphic=false)`, reusing
the existing deleted-object / outside-lifetime diagnostic notes.

## Notes

One remaining polish item, not required for filing: the reused note says
"dynamic_cast of ..." for what is actually an implicit base conversion --
worth a follow-up note variant, but not blocking.
