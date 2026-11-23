# CLANG-792: a member call through a null pointer is reported again in the callee's prologue

**Status:** Accepted by the user 2026-10-17 (DECISIONS.md E114 (b), by the
general rule E120): twice at -O0 stays (branch-only: the double report
needs the P3100 check)
**Kind:** divergence (gnu_gcc reports once)
**Upstream:** `--` (the P3100 checks are ours; the prologue check itself is
stock -fsanitize=null behaviour)
**Measured:** this branch's install of `3b0d6532ef4f` and of the M85-Clang
tree (DECISIONS.md E111 (a) applied), gnu_gcc's install of the
`333467f4592` tree, at -O0 and -O2
**Pinned by:** `clang/test/Contracts/Runnable/p3100-null-member-call-prologue.cpp`
(the former watch test, 2 at -O0 and 1 at -O2)

## Symptom

With `-fsanitize=null` routed to the contract-violation handler (as
`-fcontracts-p3100` implies) and `ub:expr.unary.dereference.nullptr`
configured, a call through a null pointer to a member function that is
not inlined is reported twice at -O0: once by the P3100 check at the call
(which replaces the sanitizer's check there), and once by the sanitizer's
check of `this` that Clang makes in every member function's prologue
(`CodeGenFunction::StartFunction`, "Check the 'this' pointer once per
function", `TCK_MemberCall`).  Forms measured: `sp->g()` (a class that is
not polymorphic), `vp->nv()` (a non-virtual function of a polymorphic
class), `(vp->*p)()` with `p` a pointer to a non-virtual member function
(DECISIONS.md E111 (a) says once), and `fp->h()` devirtualized through a
`final` class.  A virtual call that is not devirtualized faults on the
vtable pointer before it reaches the callee: once.

At -O2 the optimizer deletes the prologue check (`this` is `nonnull`),
and the call is reported once.  With the P3100 leaf of the call ignored
(only the other leaf configured), the sanitizer's check at the call is
made instead, and the prologue check still reports again at -O0.

Stock Clang's `-fsanitize=null` alone makes both checks too, and
reports such a call twice (call site and prologue) with
`-fsanitize-recover=null`.  gnu_gcc makes no prologue check: once in every
form.

## Expected

DECISIONS.md E114 (open): (a) once -- under `-fcontracts-p3100` the
prologue makes no sanitizer null check of `this`, every call compiled with
the P3100 check or the sanitizer checking the object pointer at the call;
(b) twice at -O0, as now and as stock `-fsanitize=null` alone.

## Fix direction

For (a): in `StartFunction`, add `SanitizerKind::Null` to the prologue
type check's skipped checks when `getLangOpts().ContractsP3100` (as it is
already for a captureless lambda's call operator), keeping its alignment
part; careful: with `Null` skipped `EmitTypeCheck` treats the pointer as
non-null, and the vptr check would load through a null `this`, so that
check needs its own null test (or a separate skip of the sanitizer's null
report alone, as `P3100MoveSanitizerNull` does at a call through a pointer
to member function).
