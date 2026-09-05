# CLANG-12: A contract mismatch between two friend declarations is accepted silently

**Kind:** defect
**Status:** Open
**Affects:** `-fcontracts`, C++26 and later; two friend declarations of the
same function, in the same class, carrying different contract predicates
**Workaround:** declare the function at namespace scope and befriend that
declaration; the mismatch is then diagnosed normally

## Symptom

Two friend declarations of one function may specify contradictory
preconditions and the compiler says nothing. Only one of them takes effect,
so a reader who trusts the other is misled about what the function checks.

## Trigger

```cpp
struct C {
  friend int f(int x) pre(x > 0);
  friend int f(int x) pre(x < 0);   // accepted; should be a mismatch
};
```

Pinned by `clang/test/Contracts/OpenBugs/deferred-friend-contract-mismatch.cpp`,
which is XFAILed and carries the namespace-scope control -- the identical
mismatch written outside a class IS diagnosed, which places the gap on the
friend path rather than on contract matching.

## Why it is open

Found 2026-09-05 while adding watch tests for open issues; not yet
investigated on this side.

GCC has the same defect, tracked there as GCC-27, and on that side it is a
**documented deferral rather than an oversight**: the second friend
declaration is discarded, and its deferred contract tokens with it, before
end-of-class late parsing runs, so by the time anything could compare them
there is nothing left. Whether Clang's cause is analogous is unknown --
Clang's contract matching is structured differently, and its late-parsing of
member contracts is not GCC's.

## Notes

Branch-only: contracts are not upstream in Clang, so there is nothing to file.

Mirror: `gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C`
in the gnu_gcc fork, whose xfail accounts for both of that suite's expected
failures.
