# Open Upstream Bugs

Bugs found during this implementation that reproduce on stock upstream
Clang, independent of anything in this branch. Each links to a
self-contained report-ready writeup plus a reproducer. A row is removed
(and its file deleted) once the bug is fixed on upstream main, regardless
of who fixed it or whether it was ever formally filed.

**LLVM tracks bugs in GitHub Issues**, not Bugzilla -- the move was in late
2021. `bugs.llvm.org` is archived read-only, `llvm.org/PR<n>` redirects to the
migrated issue, and migrated issues carry a `bugzilla` label plus a
"Bugzilla Link" row in their body. Triage is by label (`clang:frontend`,
`clang:codegen`, `confirmed`) rather than by component. The public search API
needs no authentication:

```
curl "https://api.github.com/search/issues?q=repo:llvm/llvm-project+is:issue+<terms>"
```

**Upstream Link** distinguishes three states: a link means an issue exists;
`None found (searched <date>)` means GitHub was actually searched and nothing
matched; `UNKNOWN` means nobody has looked yet. `--` is not used here -- every
row in this table reproduces on stock, so every row is filable, and "no link"
can only mean unfiled. It does mean something in
[`../open-issues/README.md`](../open-issues/README.md), where a
contracts-dependent issue has nothing upstream to link because contracts are
not upstream in Clang.

For what is broken on **this branch** right now -- including branch-only
issues that never reproduce upstream, which on the Clang side is most of the
contracts ones -- see
[`../open-issues/README.md`](../open-issues/README.md). That file also carries
the `Next ID` line both directories allocate from.

| Bug | Summary | Status | Upstream Link | Details |
|-----|---------|--------|----------------|---------|
| CLANG-1 | Constant evaluator accepts converting to a virtual base through an object outside its lifetime | Fixed here | None found (searched 2026-09-05) | [clang-01-constexpr-vbase-lifetime.md](clang-01-constexpr-vbase-lifetime.md) |
| CLANG-2 | Recovering from an invalid `_Bool` load leaves the invalid bits in place, where GCC coerces to `false` | Open | None found (searched 2026-09-05) | [clang-02-ubsan-bool-recover.md](clang-02-ubsan-bool-recover.md) |
| CLANG-5 | Returned object is not destroyed when a local's destructor throws during a return statement | Open | [#12658](https://github.com/llvm/llvm-project/issues/12658) | [clang-05-retval-not-destroyed-on-throwing-cleanup.md](clang-05-retval-not-destroyed-on-throwing-cleanup.md) |
| CLANG-8 | `this` accepted in the declaration of an explicit-object member function | Open | None found (searched 2026-09-05) | [clang-08-this-in-xobj-declaration.md](clang-08-this-in-xobj-declaration.md) |
| CLANG-9 | Constexpr evaluator accepts forming a non-virtual-base or direct member's address before its non-trivial constructor begins | Open | [#211286](https://github.com/llvm/llvm-project/issues/211286) (partial -- see below) | [clang-09-member-address-before-ctor.md](clang-09-member-address-before-ctor.md) |
