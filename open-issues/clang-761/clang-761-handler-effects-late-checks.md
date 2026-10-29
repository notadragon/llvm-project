# CLANG-761: handler effects are lost across a routed sanitizer check the middle end inserts

**Status:** Open here (branch-only: routing to the contract-violation
handler is a P3850 extension).  Stopped: the fix is not easy (see Fix
options); the choice is the user's, raised in the M67-Clang report
**Kind:** defect
**Upstream:** `--`
**Measured:** this branch's install of 2026-10-14 (`77a97b5653f3`), at -O0,
-O1, -O2 and -O3

## Reproducer

[`clang-761-handler-effects-late-checks.cpp`](clang-761-handler-effects-late-checks.cpp),
at `-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -O2
-fsanitize=local-bounds -fsanitize-semantic=local-bounds:noexcept_observe`
(libc++):

    static int reports;
    void handle_contract_violation(const std::contracts::contract_violation &)
    { ++reports; std::puts("H"); }
    ...
    int before = reports;
    volatile int x = a[i];                    // i == 4, a is int[4]
    std::printf("%d\n", reports - before);    // prints "H", then 0

## What goes wrong

A sanitizer check that the middle end inserts after optimizing reaches the
handler through a call the earlier passes never saw.  local-bounds is the
`BoundsChecking` pass (`registerScalarOptimizerLateEPCallback` in
`BackendUtil.cpp`, the end of the function simplification pipeline);
address, memory and thread are instrumented by their passes at the end of
the optimization pipeline (`addSanitizers`).  Before they run, GVN and the
other passes have already forwarded the value of `reports` loaded before
the access to its use after it, since nothing between could write it.  So
when the routed check reports and continues (`observe`,
`noexcept_observe`), the handler runs (it prints) but the code after the
check does not see what it did: the count printed is 0 at -O1 and above,
for `-fsanitize=local-bounds`, `address` (a heap read past the end),
`memory` (a branch on an uninitialized value) and `thread` (a race).  At
-O0 each count is 1.  The handler's call carries no memory attributes (the
`BoundsChecking` runtime call has `nounwind` only); the problem is when it
is inserted, not how it is declared.

The checks the front end emits are fine: native implicit checks (null,
alignment, division by zero; `Runnable/p3100-handler-effects-inlined.cpp`)
and routed UBSan checks (array-bounds, alignment, signed-integer-overflow,
shift, null, vptr, float-cast-overflow, bool;
`Runnable/p3100-routed-handler-effects.cpp` covers four) are counted at -O2.
`-fsanitize=bounds` includes local-bounds, so an out-of-bounds read of a
local array is reported twice under it (array-bounds and local-bounds; the
repetition is allowed, [basic.contract.eval]) and the second report's
effects are lost.

GCC has the same defect for `-fsanitize=address` and `thread` (measured on
the installed GCC; gnu_gcc GCC-806); GCC-805, fixed there, was the
front-end-emitted internal functions only.

## Expected

The count is 1 after each check, as at -O0.

## Fix options

The pass that emits the report call runs after the passes whose
assumptions it breaks, so marking the call cannot help.  (a) When the
sanitizer is routed to a semantic that calls the handler and continues,
run its instrumentation before the optimizer (pipeline start), so the
later passes see the calls -- a large slowdown for address/memory/thread
(their checks then block SROA and promotion) and less local-bounds coverage
(object sizes known only after inlining).  (b) Record it as a limitation
(a divergence row): after a routed middle-end check that continues, the
program goes on to execute the undefined access, and the handler's effects
on objects the program then reads are not guaranteed visible at -O1 and
above.  (c) As (a) for local-bounds only, (b) for the rest.

## Watch test

`clang/test/Contracts/OpenBugs/p3100-routed-handler-effects-late.cpp`
(XFAIL; one RUN line per sanitizer).

## Found by

M67-Clang (2026-10-14), measuring GCC-805's case on Clang.
