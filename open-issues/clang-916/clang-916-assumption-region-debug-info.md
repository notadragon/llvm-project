# CLANG-916: an unproven obligation's region leaves types in -g output under -fcontracts-prove-assume

**Status:** open; documented in `clang/docs/UsersManual.md` (the analysis
evaluation's description of `-fcontracts-prove-assume`)
**Kind:** deferred (a documented limitation) -- deferred by the user
2026-11-09 (DECISIONS.md E211 (c))
**Upstream:** `--` (assumption regions are ours)
**Measured:** 2026-11-09, this branch at `8e21e613c652` (the install,
release build with assertions)
**Watch test:** `clang/test/Contracts/OpenBugs/p4405-shadow-debug-info-assume.cpp`
**Found by:** PO7-Clang (D4405 follow-up program), checking gnu_gcc's
GCC-998 shapes (deferred there as a documented limitation, notadragon_wg21
DECISIONS.md E209 (c))

## Symptom

```c++
struct T { int n; T(int n) : n(n) {} ~T(); bool ok() const { return n > 0; } };
int f10(int n) { contract_assert(T(n).ok()); return 10; }
int f11(int n) { contract_assert([&] { return n > 0; }()); return 11; }
```

`clang -cc1 -std=c++26 -fcontracts -fcontracts-p4405
-debug-info-kind=constructor -fcontract-evaluation-semantic=diagnose
-emit-obj` (so `-fcontracts-prove-assume`, which `-fcontracts-p4405`
implies) against the same at `-fcontracts-prove-optimize=0`: at -O0 and at
-O2 the objects differ in `.debug_info`: `T` (with its constructor,
destructor and `ok`) is a retained type of the compile unit, and `f11`'s
retained nodes hold the lambda's closure type, though neither obligation is
proven and so no assumption is kept, and the code is the same.  The line
table does not differ.  gnu_gcc's other GCC-998 shapes do not reproduce:
no statement marker is lost in an `if`/`else`, a `switch` case or a loop
body, a function called only in a predicate gets no debug entry, and an
override shadow's snapshot leaves the body's scopes alone.

## Cause

Under `-fcontracts-prove-assume` each obligation leaves an assumption
region in the real function, `if (__clang_contract_proven(id))
llvm.assume(pred)`, emitted with the real code and so with its debug
information: the predicate's explicit conversion retains its type, and the
functions it calls (`T::T`, the lambda's call operator) are emitted with
theirs, describing their classes.  The back end deletes the region when the
proof is not sound, after the module's debug information has been
finalized by code generation.

## Options

(a) Emit each region's predicate without debug information and track the
functions first referenced from a region: emit those without debug
information unless the real code references them too, and keep the
conversions' types out of the retained list (`EmitExplicitCastExprType`).
A kept region (a sound proof) would then call functions without debug
information (allowed; their inlined code takes the call's location) -- the
E167 exception already allows kept assumptions to change the debug
information.  Touches `CodeGenModule`'s deferred emission and `CGExpr.cpp`;
moderate.

(b) Prune the retained types and nodes after the decision: not attributable
-- an upstream compilation retains the same descriptions when real code
names the type and the optimizer then deletes it.

(c) Document it as a limitation, as gnu_gcc's GCC-998 (E209 (c)).

## Decision

(c), DECISIONS.md E211, 2026-11-09: documented and left, for parity with
gnu_gcc's GCC-998, its GCC counterpart (deferred there by E209 (c)).
`UsersManual.md` says, where the analysis evaluation describes
`-fcontracts-prove-assume`, that with `-g` the debug information can
describe the types a predicate names even when its assumption is removed,
while the code and the line table are unchanged.  The watch test stays,
xfailed, as the record of the current behaviour (it XPASSes if the regions
stop leaving traces).  Revisit with (a) if identical debug information
turns out to matter.
