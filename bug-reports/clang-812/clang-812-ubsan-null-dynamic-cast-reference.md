# CLANG-812: `-fsanitize=null` does not check the operand of a `dynamic_cast` to a reference type

**Status:** Open here; open upstream.  The P3100 check on this branch
reports every form (`.access`, DECISIONS.md E102 (a); CLANG-780/781).
Where its leaf is ignored and `-fsanitize=null` is routed, DECISIONS.md
E111 (a) says the sanitizer's check is made instead -- there is none, so
those runs report 0 where gnu_gcc reports 1 (CLANG-807's mirror of
GCC-875's tests).  Whether to fix the stock sanitizer here too is
DECISIONS.md E129 (E84 / E91 / E123 precedent): deferred by the user
2026-10-19 (E129 (a)); this report, the `verify.sh` case and the watch
test stay, the upstream report stays owed
**Component:** clang:codegen
**Upstream Link:** None found (searched 2026-10-18) -- filing owed
**Affects:** measured 2026-10-18 at -O0 and -O2 -- 18.1.0, 21.1.0, 22.1.6,
the stock trunk nightly (`33891afe2ebb`) and this branch's install
(`81526623b898`): `dynamic_cast<W &>(*vp)`, `dynamic_cast<const W &>(*vp)`
and `dynamic_cast<W &>(vp[0])`, `vp` null, are not reported; the cast then
calls `__dynamic_cast` with the null pointer, which faults reading the
vtable pointer (libc++abi) or returns null so that the cast throws
`std::bad_cast` (libstdc++'s libsupc++).  A member access
through a null pointer as the operand (`hp->m`) is reported (the member
access's check).  gcc (13.4.0, trunk and gnu_gcc's install) reports each:
"reference binding to null pointer of type 'struct V'".
**D4277R1 category:** `ub:expr.unary.dereference.nullptr` (UBSan's
`-fsanitize=null`)

## Reproducer

[`clang-812-ubsan-null-dynamic-cast-reference.cpp`](clang-812-ubsan-null-dynamic-cast-reference.cpp),
at `-std=c++17 -fsanitize=null`, run: it prints the number of UBSan
reports of each form before its fault (caught with `SIGSEGV` and resumed)
or `std::bad_cast` (caught):
`star=0 cstar=0 sub=0 member=1`; gcc prints `star=1 cstar=1 sub=1
member=1`.

## Cause

For the reference form, `CodeGenFunction::EmitDynamicCast` is given the
operand's address from an unchecked lvalue (`*vp`) and type-checks it as
`TCK_DynamicOperation`, which
`isNullPointerAllowed` lists as allowing a null pointer -- right for
`dynamic_cast<W *>(vp)`, whose operand may be null, but not for the
reference form, whose operand is a glvalue: `*vp` with `vp` null is
already undefined.  So the reference form has no null check before the
call of `__dynamic_cast`.  A fix would check the operand for null when the cast's
type is a reference type (`TCK_ReferenceBinding`-like, "reference binding
to null pointer"), as gcc does.

## Upstream search (2026-10-18)

GitHub issue search on llvm/llvm-project including closed issues.
Queries: `dynamic_cast fsanitize=null`; `ubsan dynamic_cast null
reference`; `sanitize null dynamic_cast`; `dynamic_cast null pointer ubsan
segfault`; `"dynamic_cast" ubsan null`; `vptr dynamic_cast null
reference`; `ubsan "dynamic operation" null`.  None is this defect.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=null` misses `dynamic_cast<T &>(*p)` with `p`
null

**Description:**

    struct V { virtual void h() {} };
    struct W : V {};
    __attribute__((noinline)) void f(V *p) { (void)dynamic_cast<W &>(*p); }
    int main() { f(nullptr); }

    $ clang++ -fsanitize=null t.cpp && ./a.out
    terminate called after throwing an instance of 'std::bad_cast'

No report precedes the fault: the cast's operand check
(`TCK_DynamicOperation`) allows a null pointer, which is right for the
pointer form but not for `*p`, a glvalue.  GCC reports "reference binding
to null pointer of type 'struct V'".

## Found by

M94-Clang, mirroring gnu_gcc's
`g++.dg/ubsan/p3100-null-dynamic-cast-reference-sanitize-null*.C`
(GCC-875) for CLANG-807: with only `.indirection` configured, or the group
ignored, the routed sanitizer reports these forms in gnu_gcc and not here.
