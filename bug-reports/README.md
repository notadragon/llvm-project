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

**A trailing `-- filing owed` after `None found` means a report is owed**:
the writeup carries a `## Draft upstream report` (or `## Bug Report`)
section ready to file.  The user files it; nobody else does.  Replace the
marker with the link once it is filed.

**A trailing `*` marks an issue we filed ourselves** -- specifically, one
whose GitHub author is our own account, not merely one filed by someone
connected to this work -- as in the GCC fork's table. Nothing here carries one yet: #12658 was filed in 2012 and #211286 by
another reporter, both checked against the issue's author rather than
assumed. The distinction matters once we do file -- on our own issue we can
comment freely and are expected to answer questions, whereas on someone
else's the useful contribution is usually a measurement or a confirmation.

For what is broken on **this branch** right now -- including branch-only
issues that never reproduce upstream, which on the Clang side is most of the
contracts ones -- see
[`../open-issues/README.md`](../open-issues/README.md). That file also carries
the `Next ID` line both directories allocate from.

## Layout: the `clang-NN-` prefix means "do not attach this"

One directory per bug, `clang-NN/`, matching
[the GCC fork's `bug-reports/`](../../gnu_gcc/bug-reports/README.md), and
within it the **filename prefix says whether a file is for upstream or for
us**: an unprefixed name is meant to go on the issue, a prefixed one is
ours. Our numbering is internal -- "CLANG-5" means nothing to anyone
upstream -- so letting it reach a filename someone else reads only invites
confusion about what the number refers to.

**Every file here still carries the prefix**, and that is correct rather
than pending: no Clang writeup has been rewritten into submission form yet,
so none of them designates which files to attach, and marking one "attach
me" on the strength of it being the only reproducer would be a guess. The
prefix comes off a file when a report names it as an attachment.

Note that the two trackers differ here. The GCC fork's reports carry a
Bugzilla field table and an Attachments table with a description per file,
because Bugzilla has both of those as form fields. GitHub Issues has
neither: there is a title, a Markdown body, and files attached by dropping
them in. So the Clang equivalent, when written, is a body to paste rather
than a form to fill -- and Markdown works there, which it does not on
Bugzilla.

## Checking whether a row still belongs here

The removal rule above -- a row goes when upstream fixes it -- has no automatic
trigger: upstream closing an issue is silent from here, and re-running the
reproducers by hand is a thing nobody does. [`verify.sh`](verify.sh) does it
instead. It measures every reproducer against both stock upstream trunk and
this branch, and prints only what has moved since the checked-in baseline in
[`verify-expected.txt`](verify-expected.txt):

```
./verify.sh              # report anything that moved
./verify.sh --record     # re-baseline, then review the git diff
./verify.sh -v CLANG-9   # show the diagnostics behind a moved digest
```

A move in the **stock** column is the event this table cares about. A move in
the **branch** column is a regression or a fix here, and the Status column
should follow it.

Run it after every rebase, once the branch compiler is rebuilt *and* the stock
nightly refreshed -- a stale nightly answers last week's question. The
procedure is `notadragon_wg21`'s
`src/pubs/impl/p3850impl/final-passes/rebase-runbook.md`; the cases and their
flags are in [`verify-cases.txt`](verify-cases.txt). The GCC fork carries the
same script over its own reproducers; keep the two in step.

| Bug | Summary | Status | Upstream Link | Details |
|-----|---------|--------|----------------|---------|
| CLANG-1 | Constant evaluator accepts converting to a virtual base through an object outside its lifetime | Fixed here | None found (searched 2026-10-12) -- filing owed | [clang-01-constexpr-vbase-lifetime.md](clang-01/clang-01-constexpr-vbase-lifetime.md) |
| CLANG-2 | Recovering from an invalid `_Bool` load leaves the invalid bits in place, where GCC coerces to `false` | Open | None found (searched 2026-09-05) | [clang-02-ubsan-bool-recover.md](clang-02/clang-02-ubsan-bool-recover.md) |
| CLANG-5 | Returned object is not destroyed when a local's destructor throws during a return statement | Open | [#12658](https://github.com/llvm/llvm-project/issues/12658) | [clang-05-retval-not-destroyed-on-throwing-cleanup.md](clang-05/clang-05-retval-not-destroyed-on-throwing-cleanup.md) |
| CLANG-8 | `this` accepted in the declaration of an explicit-object member function | Open | None found (searched 2026-09-05) | [clang-08-this-in-xobj-declaration.md](clang-08/clang-08-this-in-xobj-declaration.md) |
| CLANG-9 | Constexpr evaluator accepts forming a non-virtual-base or direct member's address before its non-trivial constructor begins | Open | [#211286](https://github.com/llvm/llvm-project/issues/211286) (partial -- see below) | [clang-09-member-address-before-ctor.md](clang-09/clang-09-member-address-before-ctor.md) |
| CLANG-39 | `typedef int F (int) requires true;` is accepted | Fixed here | None found (searched 2026-09-25) | [clang-39-typedef-requires-clause.md](clang-39/clang-39-typedef-requires-clause.md) |
| CLANG-628 | An out-of-class definition of a member of a class template takes its parameters' top-level cv-qualifiers from the in-class declaration (dependent parameter types) | Open | [#29713](https://github.com/llvm/llvm-project/issues/29713) | [clang-628-param-cv-out-of-class-member.md](clang-628/clang-628-param-cv-out-of-class-member.md) |
| CLANG-664 | The noexcept operator ignores an initializer list's array filler: `noexcept(A{})` is true for `struct A { S s[2]; }` with a potentially-throwing `S::S()` | Fixed here | None found (searched 2026-10-09) | [clang-664-noexcept-array-filler.md](clang-664/clang-664-noexcept-array-filler.md) |
| CLANG-666 | libc++'s clang-tidy plugin registers an empty regex for a module partition with no `ExtraHeader`, so `module_std.gen.py` aborts under a clang-tidy built with assertions | Fixed here | None found (searched 2026-10-09) | [clang-666-libcxx-tidy-empty-regex.md](clang-666/clang-666-libcxx-tidy-empty-regex.md) |
| CLANG-688 | UBSan `vptr` reports a defined `static_cast` downcast performed during construction of the derived object | Open | [#59060](https://github.com/llvm/llvm-project/issues/59060) | [clang-688-ubsan-vptr-downcast-during-construction.md](clang-688/clang-688-ubsan-vptr-downcast-during-construction.md) |
| CLANG-689 | A one-past-the-end pointer converted to a non-virtual base is rejected in constant evaluation | Open | [#43201](https://github.com/llvm/llvm-project/issues/43201) (open); related #223064 | [clang-689-constexpr-past-end-nonvirtual-base.md](clang-689/clang-689-constexpr-past-end-nonvirtual-base.md) |
| CLANG-710 | An unsequenced modification and read of a scalar is accepted in constant evaluation | Open | [#37768](https://github.com/llvm/llvm-project/issues/37768) (#108424 duplicate) | [clang-710-constexpr-unsequenced-modification.md](clang-710/clang-710-constexpr-unsequenced-modification.md) |
| CLANG-711 | A `[[noreturn]]` function that returns is accepted in constant evaluation | Open | [#211272](https://github.com/llvm/llvm-project/issues/211272) | [clang-711-constexpr-noreturn-returns.md](clang-711/clang-711-constexpr-noreturn-returns.md) |
| CLANG-712 | `&*(a + N)` (indirection through a past-the-end pointer, address taken) is accepted in constant evaluation | Open | [#65119](https://github.com/llvm/llvm-project/issues/65119) (closed not_planned: a maintainer considers `&a[N]` well-defined) | [clang-712-constexpr-address-past-end-indirection.md](clang-712/clang-712-constexpr-address-past-end-indirection.md) |
| CLANG-713 | A member call (or `dynamic_cast`) on `*this` in a base-class mem-initializer is accepted in constant evaluation | Open | [#211286](https://github.com/llvm/llvm-project/issues/211286) (exact for the call; fix PR #219358 open; `dynamic_cast` partial) | [clang-713-constexpr-call-in-base-mem-init.md](clang-713/clang-713-constexpr-call-in-base-mem-init.md) |
| CLANG-714 | Uses of an object under construction through a not-yet-constructed path ([class.cdtor]) are not rejected in constant evaluation | Open | None found (searched 2026-10-12) -- filing owed | [clang-714-constexpr-cdtor-uses.md](clang-714/clang-714-constexpr-cdtor-uses.md) |
| CLANG-715 | Naming a reference whose referent's storage has been released is not rejected in constant evaluation | Open | None found (searched 2026-10-12) -- filing owed | [clang-715-constexpr-reference-to-freed-storage.md](clang-715/clang-715-constexpr-reference-to-freed-storage.md) |
| CLANG-716 | Assignment from an inexactly overlapping union member is not rejected in constant evaluation | Open | None found (searched 2026-10-12) -- filing owed | [clang-716-constexpr-union-overlap-assignment.md](clang-716/clang-716-constexpr-union-overlap-assignment.md) |
| CLANG-717 | Binding a reference through a pointer to an automatic object whose scope has ended is not rejected in constant evaluation | Open | None found (searched 2026-10-12) -- filing owed | [clang-717-constexpr-reference-through-dangling-pointer.md](clang-717/clang-717-constexpr-reference-through-dangling-pointer.md) |
| CLANG-718 | `static_cast` of an out-of-range floating value to an unfixed enumeration is accepted in constant evaluation | Open | None found (searched 2026-10-12) -- filing owed | [clang-718-constexpr-enum-floating-cast.md](clang-718/clang-718-constexpr-enum-floating-cast.md) |
| CLANG-725 | Constant evaluation gives the wrong result for a defined `dynamic_cast` (or `typeid`) through a virtual base during construction | Open | None found (searched 2026-10-12) -- filing owed | [clang-725-constexpr-dynamic-cast-vbase-construction.md](clang-725/clang-725-constexpr-dynamic-cast-vbase-construction.md) |
| CLANG-734 | `-fsanitize=vptr` does not check a member of a polymorphic base class reached through a glvalue of the wrong derived type (`reinterpret_cast<B &>(a).x`, `x` a member of a base shared by `A` and `B`) | Open | None found (searched 2026-10-13) -- filing owed | [clang-734-vptr-base-member.md](clang-734/clang-734-vptr-base-member.md) |
| CLANG-737 | A static member named through `->` on a null pointer (`sp->sf()`, `sp->k`) is accepted in constant evaluation | Open | None found (searched 2026-10-13) -- filing owed; related [#77186](https://github.com/llvm/llvm-project/issues/77186) | [clang-737-constexpr-null-static-member-call.md](clang-737/clang-737-constexpr-null-static-member-call.md) |
| CLANG-748 | `-fsanitize=null` does not check a member of a base class used through a null pointer (a base at a nonzero offset, or virtual) | Open | None found (searched 2026-10-14) -- filing owed | [clang-748-ubsan-null-base-member.md](clang-748/clang-748-ubsan-null-base-member.md) |
| CLANG-752 | `-fsanitize=vptr` does not check a data member read through a reference variable or parameter bound to a glvalue of the wrong polymorphic type | Open | None found (searched 2026-10-14) -- filing owed | [clang-752-vptr-reference-variable.md](clang-752/clang-752-vptr-reference-variable.md) |
| CLANG-756 | An indirection through a null pointer to function (`F &r = *fp`, `&*fp`, `*fp;`) is accepted in constant evaluation | Open | None found (searched 2026-10-14) -- filing owed; related [#77186](https://github.com/llvm/llvm-project/issues/77186), [#69108](https://github.com/llvm/llvm-project/issues/69108) | [clang-756-constexpr-null-function-indirection.md](clang-756/clang-756-constexpr-null-function-indirection.md) |
| CLANG-763 | `-fsanitize=null` does not check a trivial assignment operator called through a pointer (`sp->operator=(s)`) | Open | None found (searched 2026-10-14) -- filing owed; related [#203737](https://github.com/llvm/llvm-project/pull/203737) | [clang-763-ubsan-null-trivial-assign-call.md](clang-763/clang-763-ubsan-null-trivial-assign-call.md) |
| CLANG-779 | An empty class copied, moved or assigned from the element one past the end of an array (`E e2 = a[2]`, `e = a[2]`) is accepted in constant evaluation | Open | None found (searched 2026-10-16) -- filing owed | [clang-779-constexpr-empty-copy-past-end.md](clang-779/clang-779-constexpr-empty-copy-past-end.md) |
| CLANG-803 | `-fsanitize=null` does not check a downcast of `*p` to a reference (`static_cast<D &>(*p)`, its binding, a member access or call through it) with `p` null when the base is at a nonzero offset in `D` | Open | None found (searched 2026-10-17) -- filing owed | [clang-803-ubsan-null-downcast-ref-offset.md](clang-803/clang-803-ubsan-null-downcast-ref-offset.md) |
| CLANG-809 | `-fsanitize=alignment` does not check an access or a placement new through a pointer formed from a character array (`*(int *)(buf + o)`, `new (s + o) S()` since 22; `*(int *)&buf[o]`, `new (&s[o]) S()` always) | Open | [#177682](https://github.com/llvm/llvm-project/issues/177682) (the load/store regression; not ours) -- comment pending | [clang-809-ubsan-alignment-char-array.md](clang-809/clang-809-ubsan-alignment-char-array.md) |
| CLANG-812 | `-fsanitize=null` does not check the operand of a `dynamic_cast` to a reference type (`dynamic_cast<W &>(*vp)`, `vp` null) | Open | None found (searched 2026-10-18) -- filing owed | [clang-812-ubsan-null-dynamic-cast-reference.md](clang-812/clang-812-ubsan-null-dynamic-cast-reference.md) |
