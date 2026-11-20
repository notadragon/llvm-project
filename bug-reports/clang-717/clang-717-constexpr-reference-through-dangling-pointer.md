# CLANG-717: Binding a reference through a pointer to an automatic object whose scope has ended is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.unary.dereference (.indirection), basic.compound.invalid.pointer (.indirection)`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-reference-through-dangling-pointer.cpp` (XFAIL)

## Reproducer

[`clang-717-constexpr-reference-through-dangling-pointer.cpp`](clang-717-constexpr-reference-through-dangling-pointer.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool t() {
      int* p;
      { int y = 1; p = &y; }
      int& r = *p;   // UB: indirection through a dangling pointer
      (void)r;
      return true;
    }
    static_assert(t());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  The deleted-heap and past-the-end bindings are rejected since 22.

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues, control validated (a phrase from #211286's title returned it).  Queries: `constexpr "past-the-end" indirection`; `constexpr "past the end" dereference address`; `constexpr "one-past-the-end" accepted`; `constexpr dereference "past the end" not rejected`; `constexpr reference out of scope variable accepted`; `constexpr "lifetime has ended" reference binding`; `constexpr "dangling" pointer "reference" binding not diagnosed`; `CWG232`; `constexpr "one past" dereference`.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Binding a reference through a pointer to an automatic object whose scope has ended is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.unary.op]; D4277R1 category `ub:expr.unary.dereference`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  The deleted-heap and past-the-end bindings are rejected since 22.

    constexpr bool t() {
      int* p;
      { int y = 1; p = &y; }
      int& r = *p;   // UB: indirection through a dangling pointer
      (void)r;
      return true;
    }
    static_assert(t());

Found by the D4277R1 per-UB review (2026-10-12).
