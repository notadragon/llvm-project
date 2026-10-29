---
id: 1140-p2900-base-driver
subject: '[clang][contracts][P2900] driver and frontend plumbing for -fcontracts'
depends: []
regenerates: []
fixes: []
coauthors: ['Eric Fiselier <eric@efcs.ca>']
---

## Rationale
`-fcontracts` and the behaviour switch beside it, plus the one
function that decides whether a compilation wants contracts at all.

`tools::wantsCxxContracts` is the part worth reading.  It answers the
question three ways, in order: an explicit `-fno-contracts` wins outright;
`-fcontracts` turns them on; otherwise the language standard decides, and
failing that any per-paper sub-flag implies them.

**The standard list is a hazard and its comment says so.**  Every standard
at or above C++26 has to appear in it, in both its year spelling and its
provisional one, and the next one has to be added when it appears.
Omitting `c++2d` is how contracts silently stopped being enabled there
while GCC still enabled them -- the same contract was a syntax error on one
compiler and not the other.  GCC had the mirror image of the same bug in
`g++spec.cc`, where an identical list decides whether to link the runtime.
Deriving this from the standard's own value rather than matching strings is
the obvious improvement and is deliberately not attempted here.

`-fcontracts-p4299` is excluded from the implying set on purpose: it is the
C front end's flag, and implying the C++ feature from it would be wrong.

The other option is a behaviour switch for the base facility rather than
a feature -- `-fcontract-constification`, defaulting on.  It exists to turn
off a rule while measuring its cost, and its `-fno-` spelling reaches
`-cc1`.  (`-fcontract-lambda-capture-restrictions` is
`1170-p2900-base-lambdas`'s, and `-fcontract-exceptions` comes last, in
`6900-cross-paper-options`; until then the base facility reports a
predicate's exception as a violation, as the language requires.)

`Driver/ToolChains/Clang.cpp` is untouched by this commit.  The gate lives
in `CommonArgs.cpp`, which does its own standard matching, so nothing in
`ConstructJob` has to change to get `-fcontracts` forwarded; the one block
this branch does add there forwards the per-paper flags and belongs to
`6900-cross-paper-options`.

## Compile gap
Needs `1000-p2900-base-basic` for `LangOpts<"Contracts">` and the
behaviour LangOpt; it precedes this commit.

**`wantsCxxContracts` names eleven options that do not exist yet** -- the
ten per-paper flags, each introduced by its own paper's commit from
`3000-p3097` through `5200-p4301`, and the umbrella `-fcontracts-p3850`
from `2900-umbrella-option`.  All eleven come later.  Standing alone this
needs each `OPT_fcontracts_pNNNN` and `OPT_fcontracts_p3850` declared in
`Options.td`; each of those commits then replaces its stub with the real
option definition and its `LangOpts<>` binding.

The alternative -- deferring `wantsCxxContracts` to the end with the rest
of the driver reconciliation in `6900-cross-paper-options` -- was not taken
because `-fcontracts` itself would then not work until the last commit in
the series.

**Behaviour trap:** with those options stubbed, the implication chain
compiles and answers `false` for every paper flag that has not landed.  So
between here and each paper's commit, `-fcontracts-pNNNN` does not imply
`-fcontracts`, and a test relying on that implication silently compiles
without contracts rather than failing.

## Contents
- clang/include/clang/Driver/CommonArgs.h : *
- clang/include/clang/Options/Options.td : @contracts, @constification
- clang/lib/Driver/ToolChains/CommonArgs.cpp : *
- clang/test/Contracts/contracts-inactive.cpp : *
- clang/test/Contracts/contracts-std-versions.cpp : *
- clang/test/Contracts/ftm-value.cpp : *
- clang/test/Contracts/p3850-implies.cpp : *
- clang/test/Contracts/virtual-contracts-require-p3097.cpp : *
