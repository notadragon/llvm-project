# CLANG-715: Naming a reference whose referent's storage has been released is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `33891afe2ebb`; this branch's install of 2026-10-12
**D4277R1 category:** `ub:expr.type.reference.lifetime`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-reference-to-freed-storage.cpp` (XFAIL)

## Reproducer

[`clang-715-constexpr-reference-to-freed-storage.cpp`](clang-715-constexpr-reference-to-freed-storage.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr int t() {
      int* ip = new int(5);
      int& i = *ip;
      delete ip;
      int& j = i;   // UB: names a reference whose referent's storage was released
      (void)j;
      return 1;
    }
    static_assert(t());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  GCC accepts too.

## Upstream search (2026-10-12)

GitHub issue search on llvm/llvm-project including closed issues, control validated (a phrase from #211286's title returned it).  Queries: `constexpr dangling reference not diagnosed`; `constexpr reference deleted object accepted`; `"dangling reference" "constant expression"`; `constexpr reference "pointer would not be valid"`; `constexpr "lifetime" reference naming deleted`; `CWG453`.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Naming a reference whose referent's storage has been released is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([expr.type]; D4277R1 category `ub:expr.type.reference.lifetime`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  GCC accepts too.

    constexpr int t() {
      int* ip = new int(5);
      int& i = *ip;
      delete ip;
      int& j = i;   // UB: names a reference whose referent's storage was released
      (void)j;
      return 1;
    }
    static_assert(t());

Found by the D4277R1 per-UB review (2026-10-12).
