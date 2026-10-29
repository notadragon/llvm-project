# CLANG-718: `static_cast` of an out-of-range floating value to an unfixed enumeration is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.static.cast.enum.outside.range (.floating)`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-enum-floating-cast.cpp` (XFAIL)

## Reproducer

[`clang-718-constexpr-enum-floating-cast.cpp`](clang-718-constexpr-enum-floating-cast.cpp), at `-std=c++26 -fsyntax-only`:

    enum A { a0, a1, a2, a3 };                 // values [0, 3]
    constexpr A x = static_cast<A>(4.0);       // undefined: 4.0 -> 4 is outside [0, 3]

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  The integral form is rejected ("integer value 4 is outside the valid range of values [0, 3]"); [expr.static.cast] converts a floating value to the underlying type and then to the enumeration, so the same rule applies.  GCC rejects (GCC-716).

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues, control validated (a phrase from #211286's title returned it).  Queries: `constexpr enum static_cast floating`; `enum-constexpr-conversion floating`; `"outside the valid range" enum double` (no relevant hits).

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `static_cast` of an out-of-range floating value to an unfixed enumeration is accepted in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.static.cast]; D4277R1 category `ub:expr.static.cast.enum.outside.range`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  The integral form is rejected ("integer value 4 is outside the valid range of values [0, 3]"); [expr.static.cast] converts a floating value to the underlying type and then to the enumeration, so the same rule applies.  GCC rejects (GCC-716).

    enum A { a0, a1, a2, a3 };                 // values [0, 3]
    constexpr A x = static_cast<A>(4.0);       // undefined: 4.0 -> 4 is outside [0, 3]

Found by the D4277R1 per-UB review (2026-10-12).
