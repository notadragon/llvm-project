---
id: 9900-fork-metadata
subject: 'Fork metadata: README, bug reports, open issues, branch history'
depends: []
regenerates: []
fixes: []
---

## Rationale

Everything about this branch that is *about* the work rather than part of
it, gathered into the last commit so that dropping one commit leaves a
purely upstreamable series.

Four things live here.  `README.md` gains a prepended description of the
fork, the papers it implements and how to enable them, along with the
attribution: the P2900 implementation this builds on is Eric Fiselier's and
Corentin Jabot's work, and everything added here is prototype-quality, for
implementation experience rather than for production.  `bug-reports/` holds
the defects that reproduce on **stock upstream Clang** -- five of them, each
a directory of notes and reproducers, with a table naming the commit in
this branch that resolves it, plus the `verify.sh` harness that re-measures
every reproducer against both stock and this branch.  `open-issues/` holds
what is still broken *here*, whatever its origin.  `branch-history/` is the
mapping that generates this history, including the file you are reading.

**A reviewer should read none of this as compiler work.**  It is never
upstreamed and is deliberately isolated: no commit before this one may
touch any of these paths, and `check` enforces that as the clean-drop
invariant.

Two properties are worth knowing, and both are shared with GCC's tip
commit.  This one is **generated last** so that it can point backwards: the
hash column in `bug-reports/README.md` is filled in from the commits that
precede it, which is why those references are written as stable
pseudo-commit ids everywhere else and resolved to real hashes only here.
And it claims its paths **by glob rather than by enumeration**, because it
owns `branch-history/` itself -- otherwise every re-seed of the mapping
would have to re-list the files the previous seed created.

One thing is smaller here than on GCC and the reason is not a judgement
about quality.  GCC's `bug-reports/` has 82 files because contracts are
upstream there, so a contracts bug can be an upstream bug; Clang has no
upstream contracts, so a contracts defect found here is ours to fix on the
branch and is recorded in `open-issues/` or in the commit that fixed it.
Only a **non-contracts** Clang defect can appear in `bug-reports/`, and
five have.

## Compile gap

None.  Nothing here is compiled, and nothing in the compiler refers to it.

The one property that must hold is negative rather than constructive: this
commit must remain the *only* one touching `README.md`, `bug-reports/`,
`open-issues/` and `branch-history/`.  If an earlier commit ever claims one
of those paths, dropping this commit stops yielding a clean upstreamable
series -- so the constraint is machine-checked rather than left to habit.

## Contents

- README.md : *
- bug-reports/* : *
- open-issues/* : *
- branch-history/* : *
