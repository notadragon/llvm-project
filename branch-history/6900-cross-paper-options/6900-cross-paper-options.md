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

The contracts option rule lives here too, for the same reason
(`handleContractsOptionRule`, notadragon_wg21 DECISIONS.md E175-E179): it
decides, from the final values of all the options together, whether a
per-paper option given positively contradicts a final `-fno-contracts` (or,
in C, `-fno-contracts-p4299`), whether `-fno-contracts-p4451` takes part of
P3400 away, and which contracts options have no effect, and diagnoses each
in the driver, the only place the options are still visible as given.  On
a C input the umbrella turns C contracts on and implies the C per-paper
options, which the driver hands `-cc1` resolved, and a C++-only option
(`-fcontracts`, the C++-only per-paper and non-paper options) warns that
it is valid for C++ but not for C, as GCC's does.

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
- clang/lib/Driver/ToolChains/Clang.cpp : /^static void RenderContractConfigurationArgs/, @handleContractsOptionRule, @ConstructJob:0
- clang/include/clang/Basic/DiagnosticDriverKinds.td : #1:1
- clang/test/Contracts/C/c-contract-options-no-effect-contracts-on.c : *
- clang/test/Contracts/C/c-contract-options-no-effect-default-value.c : *
- clang/test/Contracts/C/c-contract-options-no-effect-negative.c : *
- clang/test/Contracts/C/c-contract-options-no-effect-p4299-off.c : *
- clang/test/Contracts/C/c-contract-options-no-effect.c : *
- clang/test/Contracts/C/c-contract-options-p3100-implies-p4299.c : *
- clang/test/Contracts/C/c-contract-options-p4298-implies-p4299.c : *
- clang/test/Contracts/C/c-contract-options-p4299-last.c : *
- clang/test/Contracts/C/c-contract-options-p4299-off-after.c : *
- clang/test/Contracts/C/c-contract-options-cxx-only.c : *
- clang/test/Contracts/C/c-contract-options-p3850-driver.c : *
- clang/test/Contracts/C/c-contract-options-p3850-implied-off.c : *
- clang/test/Contracts/C/c-contract-options-p3850-implied-p4298.c : *
- clang/test/Contracts/C/c-contract-options-p3850-link.c : *
- clang/test/Contracts/C/c-contract-options-p3850-implies-p4299.c : *
- clang/test/Contracts/C/c-contract-options-p3850-p4299-off.c : *
- clang/test/Contracts/C/c-contract-options-p4299-off-before.c : *
- clang/test/Contracts/C/c-contract-options-p4299-off-negative.c : *
- clang/test/Contracts/C/c-contract-options-p4299-off-paper-off.c : *
- clang/test/Contracts/C/c-contract-options-p4405-implies-p4299.c : *
- clang/test/Contracts/C/c-contract-options-p4405-sub.c : *
- clang/test/Contracts/C/c-contract-options-violation-information.c : *
- clang/test/Contracts/Runnable/contract-options-additive-link-p3098-no-p4405.cpp : *
- clang/test/Contracts/Runnable/contract-options-additive-link-p3400-no-p3850.cpp : *
- clang/test/Contracts/Runnable/contract-options-additive-link-p3850-no-p3400.cpp : *
- clang/test/Contracts/Runnable/contract-options-additive-link-p4451-no-p3400.cpp : *
- clang/test/Contracts/contract-options-additive-ftm-p3400-no-p3850.cpp : *
- clang/test/Contracts/contract-options-additive-ftm-p3850-no-p3400.cpp : *
- clang/test/Contracts/contract-options-additive-nnn.cpp : *
- clang/test/Contracts/contract-options-additive-nnx.cpp : *
- clang/test/Contracts/contract-options-additive-nny.cpp : *
- clang/test/Contracts/contract-options-additive-no-contracts.cpp : *
- clang/test/Contracts/contract-options-additive-nxn.cpp : *
- clang/test/Contracts/contract-options-additive-nxx.cpp : *
- clang/test/Contracts/contract-options-additive-nxy.cpp : *
- clang/test/Contracts/contract-options-additive-nyn.cpp : *
- clang/test/Contracts/contract-options-additive-nyx-rev.cpp : *
- clang/test/Contracts/contract-options-additive-nyx.cpp : *
- clang/test/Contracts/contract-options-additive-nyy.cpp : *
- clang/test/Contracts/contract-options-additive-xnn.cpp : *
- clang/test/Contracts/contract-options-additive-xnx.cpp : *
- clang/test/Contracts/contract-options-additive-xny-rev.cpp : *
- clang/test/Contracts/contract-options-additive-xny.cpp : *
- clang/test/Contracts/contract-options-additive-xxn.cpp : *
- clang/test/Contracts/contract-options-additive-xxx.cpp : *
- clang/test/Contracts/contract-options-additive-xxy.cpp : *
- clang/test/Contracts/contract-options-additive-xyn-rev.cpp : *
- clang/test/Contracts/contract-options-additive-xyn.cpp : *
- clang/test/Contracts/contract-options-additive-xyx.cpp : *
- clang/test/Contracts/contract-options-additive-xyy.cpp : *
- clang/test/Contracts/contract-options-additive-ynn-rev.cpp : *
- clang/test/Contracts/contract-options-additive-ynn.cpp : *
- clang/test/Contracts/contract-options-additive-ynx-rev.cpp : *
- clang/test/Contracts/contract-options-additive-ynx.cpp : *
- clang/test/Contracts/contract-options-additive-yny.cpp : *
- clang/test/Contracts/contract-options-additive-yxn-rev.cpp : *
- clang/test/Contracts/contract-options-additive-yxn.cpp : *
- clang/test/Contracts/contract-options-additive-yxx.cpp : *
- clang/test/Contracts/contract-options-additive-yxy.cpp : *
- clang/test/Contracts/contract-options-additive-yyn.cpp : *
- clang/test/Contracts/contract-options-additive-yyx.cpp : *
- clang/test/Contracts/contract-options-additive-yyy.cpp : *
- clang/test/Contracts/contract-options-rule-contracts-last.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-after.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-before.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-cxx26.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-every-paper.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-negative.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-p4451.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-paper-off.cpp : *
- clang/test/Contracts/contract-options-rule-no-contracts-umbrella.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-contracts-on.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-default-value.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-negative-last.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-negative-no-contracts.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-negative.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-no-contracts.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-positive-last.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect-violation-information.cpp : *
- clang/test/Contracts/contract-options-rule-no-effect.cpp : *
- clang/test/Contracts/contract-options-rule-p4405-sub-negative.cpp : *
- clang/test/Contracts/contract-options-rule-p4405-sub-no-contracts.cpp : *
- clang/test/Contracts/contract-options-rule-p4405-sub-quiet.cpp : *
- clang/test/Contracts/contract-options-rule-p4405-sub-umbrella-off.cpp : *
- clang/test/Contracts/contract-options-rule-p4405-sub.cpp : *
- clang/test/Contracts/fno-contracts-subflag-driver.cpp : *
- clang/test/Contracts/std-module-contracts-names.cpp : *
- clang/test/Contracts/std-module-contracts-labels.cpp : *
