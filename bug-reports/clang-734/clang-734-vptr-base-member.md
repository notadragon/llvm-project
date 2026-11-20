# CLANG-734: `-fsanitize=vptr` does not check a member of a polymorphic base reached through the wrong derived type

**Status:** Open here; open upstream.  Deferred by the user 2026-10-13
(DECISIONS.md E76)
**Component:** compiler-rt:ubsan / clang:codegen
**Upstream Link:** None found (searched 2026-10-13) -- filing owed
**Affects:** measured 2026-10-13 -- 18.1.0, 22.1.6, the stock trunk nightly
`33891afe2ebb` and this branch's install.  GCC the same (gnu_gcc GCC-773).
**D4277R1 category:** `ub:expr.ref.member.not.similar` (the polymorphic
part, which `-fsanitize=vptr` covers and P3100 routes)
**Watch test:** `clang/test/Contracts/OpenBugs/vptr-base-member.cpp` (XFAIL)

## Reproducer

[`clang-734-vptr-base-member.cpp`](clang-734-vptr-base-member.cpp), at
`-fsanitize=vptr`, run:

    clang-734-vptr-base-member.cpp:8:50: runtime error: member access within address 0x... which does not point to an object of type 'B'

That is `own`, a member of `B` read through a `B` glvalue that refers to an
`A`.  `inherited`, which reads `Base::x` through the same `B` glvalue, is
not reported, though [expr.ref] makes it undefined in the same way: the
object expression's type `B` is not similar to the object's type `A`, and
`B` is not a base of `A`.

## Cause

The member access's object expression is the `Base` subobject:
`MemberExpr .x` over `ImplicitCastExpr <UncheckedDerivedToBase (Base)>` over
the `B` glvalue.  `EmitMemberExpr` checks that base expression
(`TCK_MemberAccess` in `EmitTypeCheck`), so the dynamic type is tested
against `Base`, which an `A` satisfies; the `B` the access went through is
never tested.  A fix would also check the class of the operand of the
derived-to-base conversion.

## Upstream search (2026-10-13)

GitHub issue search on llvm/llvm-project including closed issues, control
validated (`is:issue vptr static_cast constructor` returned #59060, CLANG-688's
issue).  Queries: `vptr base member`; `fsanitize=vptr member base class`;
`vptr "member access within" base`; `vptr reinterpret_cast base class
member`; `sanitize vptr inherited member`; `is:issue ubsan vptr false
negative`; `is:issue vptr check base subobject`; `is:issue "-fsanitize=vptr"
member`.  None is this defect.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=vptr` does not check a base class member accessed
through the wrong derived type

**Description:**

    struct Base { virtual ~Base() {} int x = 1; };
    struct A : Base { int a = 2; };
    struct B : Base { int b = 3; };
    __attribute__((noinline)) int own(A &r) { return reinterpret_cast<B &>(r).b; }
    __attribute__((noinline)) int inherited(A &r) { return reinterpret_cast<B &>(r).x; }
    int main() { A a; inherited(a); own(a); }

    $ clang++ -fsanitize=vptr t.cpp && ./a.out
    t.cpp:4:50: runtime error: member access within address 0x... which does not point to an object of type 'B'

Only `own()` is reported.  `inherited()` reads `Base::x` through a `B`
glvalue that refers to an `A`, which [expr.ref] makes undefined just the
same; the check is made against the `Base` subobject, which an `A` has.
GCC behaves the same way.

## Found by

The M51-Clang mirror (CLANG-730) of gnu_gcc GCC-773, found by M48-GCC's
work on `ub:expr.ref.member.not.similar` (D4277R1 review F16).
