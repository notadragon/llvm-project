---
id: 2900-umbrella-option
subject: '[clang][contracts] add the -fcontracts-p3850 umbrella option'
depends: []
regenerates: []
fixes: []
---

## Rationale
`-fcontracts-p3850`, and nothing else.

The umbrella turns on every extension this fork implements at once.  It is
the flag a user of the fork actually types, and it is separated out for two
reasons that pull in the same direction: it is the one option that is not
upstreamable under any circumstances -- no paper proposes it -- and a
reviewer taking only some of this series needs it to be a single commit
they can drop.

**The per-paper `-fcontracts-pNNNN` options are deliberately NOT here.**
Each rides with the commit that implements its paper, so that taking a
paper takes its flag.  The consequence is that this commit's help text
promises implications that nothing yet performs: the reconciliation of the
umbrella against the ten sub-flags lives in the driver, and that block is
`6900-cross-paper-options`, last in the series because it is the one place
where every paper's flag is named at once.

**It sits immediately before the first paper, and not earlier.**  It is not
inside the base facility: `-fcontracts` -- the standard feature's own flag
-- belongs to `1140-p2900-base-driver`, and the two are different things.
One enables C++26 contracts; this one enables our extensions to them, so it
belongs after the facility it extends rather than in front of it.

It is not later either, and that is GCC's constraint rather than Clang's.
On GCC each per-paper option carries
`LangEnabledBy(C++ ObjC++,fcontracts-p3850)` on its own `c.opt` line, so
the umbrella has to be declared before the first paper or nine commits each
stub it.  Clang has no such coupling -- its per-paper options never name
the umbrella -- but the band number is a shared contract between the two
mappings, so it matches.

## Compile gap
None.  `LangOpts<"ContractsP3850">` is declared by
`1000-p2900-base-basic`'s `LangOptions.def`, which now precedes this
commit, and nothing else here is forward-referenced.

**Behaviour trap:** the flag compiles, parses and sets its option, and does
nothing whatsoever.  Nothing reads `ContractsP3850` until the driver block
in `6900-cross-paper-options` lands, so between here and there
`-fcontracts-p3850` is accepted in silence and implies none of the nine
papers its own help text names.

## Contents
- clang/include/clang/Options/Options.td : @p3850
