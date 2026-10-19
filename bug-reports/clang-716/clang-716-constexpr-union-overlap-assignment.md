# CLANG-716: Assignment from an inexactly overlapping union member is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.assign.overlap`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-union-overlap-assignment.cpp` (XFAIL)

## Reproducer

[`clang-716-constexpr-union-overlap-assignment.cpp`](clang-716-constexpr-union-overlap-assignment.cpp), at `-std=c++26 -fsyntax-only`:

    union U { int i; short s; };
    constexpr int f() {
      U u;
      u.s = 1;
      u.i = u.s;    // UB: stored value read from an inexactly overlapping object
      return u.i;
    }
    static_assert(f() == 1);

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  GCC accepts too.

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues, control validated (a phrase from #211286's title returned it).  Queries: `constexpr union overlap assignment`; `constexpr "overlapping" assignment union`; `union member assigned from other member constexpr`; `"expr.ass" overlap`.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Assignment from an inexactly overlapping union member is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.assign]; D4277R1 category `ub:expr.assign.overlap`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  GCC accepts too.

    union U { int i; short s; };
    constexpr int f() {
      U u;
      u.s = 1;
      u.i = u.s;    // UB: stored value read from an inexactly overlapping object
      return u.i;
    }
    static_assert(f() == 1);

Found by the D4277R1 per-UB review (2026-10-12).
