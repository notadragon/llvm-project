# CLANG-752: `-fsanitize=vptr` does not check a data member read through a reference variable

**Status:** Open here; open upstream.  Deferral proposed (notadragon_wg21
DECISIONS.md E87)
**Component:** clang:codegen
**Upstream Link:** None found (searched 2026-10-14) -- filing owed
**Affects:** measured 2026-10-14 -- 18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6,
the stock trunk nightly `33891afe2ebb` and this branch's install
(`7716d65bf8d3`), at -O0 and -O2.  GCC reports every form (gnu_gcc
`g++.dg/ubsan/vptr-reinterpret-reference.C`, GCC-798).
**D4277R1 category:** `ub:expr.ref.member.not.similar` (the polymorphic
part, which `-fsanitize=vptr` covers and P3100 routes)
**Watch test:** `clang/test/Contracts/OpenBugs/vptr-reinterpret-reference.cpp`
(XFAIL)

## Reproducer

[`clang-752-vptr-reference-variable.cpp`](clang-752-vptr-reference-variable.cpp),
at `-fsanitize=vptr`, run:

    clang-752-vptr-reference-variable.cpp:7:53: runtime error: member access within address 0x... which does not point to an object of type 'B'

That is `direct`, a member of `B` read through `reinterpret_cast<B &>(a)`,
where `a` is an `A` and the two polymorphic classes are unrelated.  `viaref`
(`B &b = reinterpret_cast<B &>(a); return b.j;`) and `param` (the same cast
bound to a `B &` parameter, `return b.j;`) read the same member through the
same glvalue and are not reported, though [expr.ref] makes each undefined in
the same way.  An rvalue reference variable is missed too; a pointer
(`B *b = reinterpret_cast<B *>(&a); return b->j;`) and a member function
call through the reference (`b.f()`) are reported.  With
`-fcontracts-p3100` on this branch the routed violation is missed the same
way: the routed report is this check.

## Cause

`EmitCheckedLValue` (`clang/lib/CodeGen/CGExpr.cpp`) emits no type check at
all for an object expression that is a `DeclRefExpr`:

    if (!isa<DeclRefExpr>(E) && !LV.isBitField() && LV.isSimple()) {

This dates from "[ubsan] Reduce null checking of C++ object pointers
(PR27581)" (34b1fd6aaaf7, 2017): a named variable needs no null check.
That holds for an object, whose type is its declared type, but a reference
names whatever it was bound to, and the `vptr` check is skipped with the
null check.  The reference binding itself is not a `vptr` check site
(`isVptrCheckRequired` excludes `TCK_ReferenceBinding`), so the wrong type
is never tested.  A fix would keep the `vptr` check (only) when the
`DeclRefExpr` names a reference; `EmitMemberExpr`'s `->` path already keeps
it for a pointer variable and skips only the null check.

## Upstream search (2026-10-14)

GitHub issue search on llvm/llvm-project including closed issues, control
validated (`is:issue vptr static_cast constructor` returned #59060, CLANG-688's
issue).  Queries: `vptr reference`; `vptr reinterpret_cast reference`;
`"-fsanitize=vptr" reference variable`; `vptr "member access within"
reference`; `ubsan vptr false negative`; `vptr DeclRefExpr`; `vptr reference
parameter member`; `sanitize vptr missed`; `vptr not detected`; `vptr check
skipped`; `ubsan vptr reference type mismatch`; `"vptr" "reinterpret_cast"`.
None is this defect.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=vptr` does not check a member access through a
reference variable

**Description:**

    struct A { virtual ~A() {} int i = 1, j = 2; };
    struct B { virtual ~B() {} int i = 1, j = 2; };
    __attribute__((noinline)) int direct(A &a) { return reinterpret_cast<B &>(a).j; }
    __attribute__((noinline)) int viaref(A &a) { B &b = reinterpret_cast<B &>(a); return b.j; }
    __attribute__((noinline)) int param(B &b) { return b.j; }
    int main() { A a; direct(a); viaref(a); param(reinterpret_cast<B &>(a)); }

    $ clang++ -fsanitize=vptr t.cpp && ./a.out
    t.cpp:3:53: runtime error: member access within address 0x... which does not point to an object of type 'B'

Only `direct()` is reported.  `viaref()` and `param()` read `B::j` through a
`B` glvalue that refers to an `A`, which is undefined just the same.
`EmitCheckedLValue` skips every type check of a `DeclRefExpr` object
expression (since 34b1fd6aaaf7, PR27581), which is right for the null check
of a variable but drops the `vptr` check of a reference.  Clang 18 to trunk;
GCC reports all three.

## Found by

M-Docs (2026-10-14), measuring D4277R1's `expr.ref.member.not.similar`
celinks; filed by M63-Clang.
