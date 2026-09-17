---
id: 6900-cross-paper-options
subject: '[clang][contracts] option-registry residue'
depends:
  - 2900-umbrella-option
  - 3000-p3097
  - 3300-p3290
  - 3600-p3099
  - 4000-p3098
  - 4200-p3400-core
  - 4600-p4283
  - 4800-p4298
  - 5000-p4299
  - 5200-p4301
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale
The option-registry residue that genuinely cannot be divided by paper.

Almost all of Clang's registry files split cleanly -- `Options.td` gives one
segment per option, `TokenKinds.def` and `IdentifierTable.cpp` cut per
feature, and `InitPreprocessor.cpp` cuts nine ways, one per feature-test
macro.  `Driver/ToolChains/Clang.cpp` does not.  Its contracts block decides
whether contracts are enabled at all -- from the language standard, from
`-fcontracts`, from the umbrella, and from any per-paper sub-flag -- and
then forwards the resolved set to `-cc1`.  The papers are not separable
there because reconciling them against each other IS what the block does.

This is the same shape as GCC's `6900`, arrived at for a different reason:
GCC's residue survived because `.opt`'s format resisted cutting, Clang's
because the logic is genuinely cross-cutting.

## Compile gap
Every flag this block forwards -- the nine per-paper ones and the umbrella
from `2900-umbrella-option` -- is introduced by an earlier commit, so
ordering it last means it needs no stubs.  Ordering it earlier would need
each stubbed in turn, which is why it is last.  Those eleven commits are
its `depends`, matching what GCC's `6900` records.

## Contents
- clang/lib/Driver/ToolChains/Clang.cpp : #0
