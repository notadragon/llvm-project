# CLANG-748: `-fsanitize=null` does not check a member of a base class used through a null pointer

**Status:** Open here; open upstream.  The P3100 check on this branch,
CLANG-747, is fixed here (M59-Clang: the object is checked before its
conversion to the base, under `-fcontracts-p3100` only); whether to fix the
stock sanitizer here too is DECISIONS.md E84
**Component:** clang:codegen
**Upstream Link:** None found (searched 2026-10-14) -- filing owed
**Affects:** measured 2026-10-14 -- 18.1.0, 22.1.6, the stock trunk nightly
`33891afe2ebb` and this branch's install, at -O0 and -O2.  GCC reports every
form (gcc-trunk `-fsanitize=null`: "member access within null pointer of
type 'struct D'").
**D4277R1 category:** `ub:expr.unary.dereference.nullptr` (UBSan's
`-fsanitize=null`)

## Reproducer

[`clang-748-ubsan-null-base-member.cpp`](clang-748-ubsan-null-base-member.cpp),
at `-fsanitize=null`, run with no argument (a base at a nonzero offset) and
with one (a virtual base): each faults with SIGSEGV, and no "member access
within null pointer" is printed.

## Cause

A member of a base class named through a pointer to a derived class, `p->b`,
is a member access whose object expression is the base subobject:
`MemberExpr ->b` over `ImplicitCastExpr <UncheckedDerivedToBase (B)>` over
`p`.  `EmitMemberExpr` emits the base expression first -- the conversion,
`GetAddressOfBaseClass` with no null check -- and then checks the *converted*
pointer (`TCK_MemberAccess` in `EmitTypeCheck`).  For a base at a nonzero
offset the converted pointer is `p + 4`, which is not null; for a virtual
base the conversion loads the base offset through `p`'s vptr, which faults
before the check.  The same holds for `(*p).b`, a member call `p->f()`, a
reference bound to `*p` as a `B &`, and `static_cast<B &>(*p).b`; a base at
offset 0 is reported, by accident of its offset.  A fix would check the
operand of the derived-to-base conversion, before converting.

## Upstream search (2026-10-14)

GitHub issue search on llvm/llvm-project including closed issues.  Queries:
`sanitize=null virtual base`; `null pointer virtual base ubsan`; `ubsan
virtual base null member access`; `fsanitize=null virtual`; `UBSan "virtual
base" null`; `"member access within null pointer" virtual`; `sanitizer null
"virtual base" segfault`; `ubsan "virtual base"`.  None is this defect
(#32568 is the vptr check reading an uninitialized vptr, fixed in 2017).

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=null` misses a member of a base class accessed
through a null pointer (nonzero-offset or virtual base)

**Description:**

    struct B0 { int z; };
    struct B { int b; };
    struct D : B0, B { int x; };      // B at offset 4
    struct VB : virtual B { int x; };
    __attribute__((noinline)) int nonvirtual(D *p) { return p->b; }
    __attribute__((noinline)) int virt(VB *p) { return p->b; }
    int main(int argc, char **) { return argc > 1 ? virt(nullptr) : nonvirtual(nullptr); }

    $ clang++ -fsanitize=null t.cpp && ./a.out
    UndefinedBehaviorSanitizer:DEADLYSIGNAL
    ==...==ERROR: UndefinedBehaviorSanitizer: SEGV on unknown address 0x000000000004 ...

No "member access within null pointer of type 'D'" is reported, for either
function: the null check is applied to the pointer after its conversion to
`B *`, which is `0x4` for a base at a nonzero offset, and for a virtual base
the conversion itself loads the base offset through the null pointer first.
`(*p).b`, `p->f()` and binding `B &` to `*p` behave the same; a base at
offset 0 is caught.  GCC reports all of them.

## Found by

M59-Clang, checking the provenance of CLANG-747 (the P3100 null check, which
shares `EmitTypeCheck` with `-fsanitize=null`).
