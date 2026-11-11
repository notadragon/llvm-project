# CLANG-763: `-fsanitize=null` does not check a trivial assignment operator called through a pointer

**Status:** Open here; open upstream.  The P3100 check on this branch,
CLANG-760, is fixed here (M67-Clang: under `-fcontracts-p3100` only, as
CLANG-747 was for CLANG-748); whether to fix the stock sanitizer here too is
owed to the user, as DECISIONS.md E84 was for CLANG-748
**Component:** clang:codegen
**Upstream Link:** None found (searched 2026-10-14) -- filing owed; related
[#203737](https://github.com/llvm/llvm-project/pull/203737) (merged; checks
the left operand of a trivial `operator=` "for the LHS (`lhs->` not handled
yet)")
**Affects:** measured 2026-10-14 -- 18.1.0, 22.1.6, the stock trunk nightly
`33891afe2ebb` and this branch's install, at -O0 and -O2.  GCC reports it
(gcc-trunk `-fsanitize=null`: "store to null pointer of type 'struct S'").
**D4277R1 category:** `ub:expr.unary.dereference.nullptr` (UBSan's
`-fsanitize=null`)

## Reproducer

[`clang-763-ubsan-null-trivial-assign-call.cpp`](clang-763-ubsan-null-trivial-assign-call.cpp),
at `-fsanitize=null`: the program faults with SIGSEGV, and no "store to null
pointer" is printed.

    struct S { int m; };
    void f(S *sp, S &s) { sp->operator=(s); }   // not reported

## Cause

`EmitCXXMemberOrOperatorMemberCallExpr` (`CGExprCXX.cpp`) emits a trivial
copy or move assignment as an aggregate copy into `getLValueForThis(true)`.
For `(*sp).operator=(s)` and `*sp = s` that is `EmitCheckedLValue(Base,
TCK_Store)`, which checks the pointer; for `sp->operator=(s)` it is
`EmitPointerWithAlignment(Base)` with no check -- the lambda carries
"FIXME: Respect EmitCheckedForStore for the IsArrow case".  A fix would
type-check the pointer as a store there.

## Upstream search (2026-10-14)

GitHub search on llvm/llvm-project, issues and pull requests, including
closed ones.  Queries: `fsanitize=null operator=`; `sanitize null
"operator=" pointer`; `ubsan trivial assignment null pointer`;
`EmitCheckedForStore` (found #203737, which names the gap, and #164548);
`ubsan "store to null pointer" member call operator`; `UBSan explicit
operator= call null`.  No issue covers it.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** `-fsanitize=null` misses `p->operator=(x)` for a trivial
assignment operator

**Description:**

    struct S { int m; };
    [[gnu::noinline]] void f(S *sp, S &s) { sp->operator=(s); }
    int main() { S *volatile vs = nullptr; S s{}; f(vs, s); }

At `-fsanitize=null` this faults with no report; `(*sp).operator=(s)` and
`*sp = s` report "store to null pointer of type 'S'".  The `IsArrow` path
of `getLValueForThis` in `EmitCXXMemberOrOperatorMemberCallExpr` does not
honour `EmitCheckedForStore` (the FIXME there; #203737 left it).  GCC
reports it.

## Found by

M67-Clang (2026-10-14), measuring CLANG-760 (the P3100 check) on stock.
