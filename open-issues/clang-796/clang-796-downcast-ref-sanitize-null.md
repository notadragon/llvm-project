# CLANG-796: a downcast of `*p` to a reference reported twice with `-fsanitize=null` routed

**Status:** Accepted under DECISIONS.md E120 (M89-Clang, 2026-10-17):
twice stays (branch-only: the double report needs the P3100 check, or the
routing); see Decision below
**Kind:** divergence (gnu_gcc reports once where a null leaf applies)
**Upstream:** `--` (the P3100 checks are ours; stock `-fsanitize=null`
alone reports these twice too, two sanitizer checks of the same pointer)
**Measured:** this branch's install of `babedfbfeb89`, gnu_gcc's install of
the M86-GCC tree (`715ebf596ed`), at -O0 and -O2, every leaf
**Pinned by:** `clang/test/Contracts/Runnable/p3100-null-downcast-ref-sanitize-null.cpp`
(the former watch test) and `Runnable/p3100-null-sanitize-null-once.cpp`

## Symptom

With `-fsanitize=null` routed to the contract-violation handler (as
`-fcontracts-p3100` implies) and `ub:expr.unary.dereference.nullptr`
configured, a downcast of `*p` to a reference, `p` null, is reported
twice:

```c++
struct V { virtual void h() {} int v; };
struct D : V { int d; void dn() {} };
D &dref(V *vp) { return static_cast<D &>(*vp); }     // 2
int drdc(V *vp) { return static_cast<D &>(*vp).d; }  // 2
void dcall(V *vp) { static_cast<D &>(*vp).dn(); }    // 2 (-O2), 3 (-O0)
```

and the same for a class that is not polymorphic
(`static_cast<DS &>(*sp)`, `static_cast<DS &>(*sp).t`).  One report is the
P3100 check of the use -- the reference binding (`.indirection`), the
member access (`.access` when read) or the member call -- which replaces
the sanitizer's check of that use; the other is the sanitizer's null check
of the downcast itself (`EmitTypeCheck(TCK_DowncastReference, ...)` in
`CodeGenFunction::EmitCastLValue`, `CK_BaseToDerived`), which nothing
replaces.  At -O0
`dcall` is reported a third time by the callee's prologue check of `this`
(CLANG-792).  With the use's P3100 leaf ignored, both sanitizer checks are
made: twice, as stock `-fsanitize=null` alone ("downcast of null pointer"
and "reference binding to" / "member access within null pointer").  A
downcast of a pointer, `static_cast<D *>(vp)->d`, is reported once (a null
pointer may be downcast).  Without `-fsanitize=null` each form is reported
once.

gnu_gcc reports the reads (`drdc`, the non-polymorphic read) twice and the
other forms once (GCC-860).

## Expected

DECISIONS.md E112 (open): does E111 (a)'s rule -- with routed
`-fsanitize=null` too, a null dereference is reported once, the P3100
check replacing the sanitizer's, which is kept where the P3100 leaf is
`assume` or `ignore` -- cover forms other than a call through a pointer to
member function?  (a) yes: once where the leaf is configured.  (b) only
pmf calls: twice stays.

## Fix direction

For (a): where the downcast's operand is the glvalue `*p` and the use's
P3100 check of the same pointer is made, the downcast's sanitizer null
check moves with it (as `P3100MoveSanitizerNull` does for a pmf call):
made only where the use's leaf resolves to `assume`/`ignore`, and then
once (the use's sanitizer check skipped).

## Decision (M89-Clang, 2026-10-17)

DECISIONS.md E120, the general report-once rule: report once where the
later check can be suppressed with no new API between subsystems and no
extra tracked state; otherwise twice is accepted.  Here the downcast's
sanitizer check is emitted first, inside the use's emission of its object
expression, and the use's P3100 check -- whose leaf (`.access` for a read,
`.indirection` for a binding or a non-virtual call) decides whether it is
made -- comes after.  Dropping the downcast's check only where the use's
P3100 check is made needs the use's resolved leaf carried to the downcast
(a new per-expression slot, as `P3100BaseConversionUse` carries a base
conversion's use), and dropping it unconditionally would lose the only
check where the use makes none or its leaf is ignored (a discarded
downcast, `&static_cast<D &>(*p)` with `.indirection` ignored) -- not
"at least once in all cases".  So the suppression is not stateless: twice
is accepted, as gnu_gcc accepted the downcast reads twice with the group
ignored (M88-GCC).  A downcast with the base at a nonzero offset is
reported once (CLANG-802's fix; the downcast's sanitizer check tests the
derived object's address there, CLANG-803).
