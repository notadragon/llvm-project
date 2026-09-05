# CLANG-13: instantiating a contract from inside a contract predicate asserts

**Status:** Open (defect)
**Kind:** defect
**Component:** Sema / contracts
**Upstream Link:** `--` (contracts are not upstream in Clang; nothing to file)
**Introduced:** `0806c848467e` (2026-09-04), "[clang][contracts] Instantiate a
function's contracts when it is odr-used"
**Found:** 2026-09-05, by the first BDE rebuild since 2026-09-01 -- 42 frontend
crashes across `bslstl-iface`, all this one assertion.

## Symptom

```
clang/lib/Sema/SemaContract.cpp:3477:
  void clang::Sema::PushContractScope(...):
  Assertion `LastScope && !LastScope->isInContract()' failed.
```

## Reproducer

[`clang-13-contract-instantiation-reentrancy.cpp`](clang-13-contract-instantiation-reentrancy.cpp),
twelve lines:

```c++
template <class T>
struct S {
  bool ok () const pre (n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T>
int f (S<T> s) pre (s.ok ()) { return s.n; }

int main () { return f (S<int>{}); }
```

```
clang++ -std=c++26 -fcontracts -fsyntax-only clang-13-...cpp
```

Nothing P3850-specific: plain `-fcontracts` is enough.

## Root cause

`InstantiateFunctionContractsOnUse` is called unconditionally from
`MarkFunctionReferenced` whenever `OdrUse == OdrUseContext::Used`
(`SemaExpr.cpp:19191`). That call site is reachable *while a contract
predicate is itself being transformed*: transforming `f`'s `pre (s.ok ())`
odr-uses `S<int>::ok`, which has its own contract, so the instantiation
recurses.

`PushContractScope` then asserts that the enclosing function scope is not
already inside a contract -- and it is, because the outer contract's transform
has not finished. The stack shows the recursion plainly, two full turns of
`TransformContractStmt` -> `SubstStmt` -> `InstantiateContractSpecifier` ->
`InstantiateFunctionContractsOnUse` -> `MarkFunctionReferenced` ->
`TransformContractStmt`.

So the assertion is not wrong about its invariant; the new odr-use hook
violates it. A fix has to either defer the nested instantiation until the
outer contract's transform completes, or make contract scopes properly
nestable and drop the invariant. Which of those is right is a design question
about `ContractScopeStack`, not a local patch, and is why this is filed rather
than fixed on the spot.

## Not caused by the 2026-09-05 rebase

Established before recording:

* the introducing commit is dated 2026-09-04, one day *before* the rebase, and
  the rebase replayed it unchanged (`range-diff`: 123 of 124 commits
  identical, the one difference trailing context in `libcxx/include/version`);
* `InstantiateFunctionContractsOnUse` does not exist before that commit, so
  the crashing call path could not have existed either;
* BDE had not been rebuilt since 2026-09-01, so this is simply the first build
  that could have seen it.

## GCC

**Not affected.** The same reproducer compiles clean on this branch's GCC with
`-fcontracts` and with `-fcontracts-p3850`. GCC instantiates contracts by a
different route, so there is no mirror bug to file -- but the watch test is
mirrored into the GCC suite anyway, per the convention, so that the two suites
keep asking the same questions.

## Impact

Blocks the whole `local-clang` BDE build at `dbg_64_cpp26_contracts`: 42
frontend crashes in `bslstl-iface`, which is core, so essentially everything
downstream of it is unbuildable. The previous recorded state for that
combination was **0 build failures**.
