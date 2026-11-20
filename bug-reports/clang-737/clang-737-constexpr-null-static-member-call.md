# CLANG-737: A static member named through `->` on a null pointer is accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-13) -- filing owed; related
[#77186](https://github.com/llvm/llvm-project/issues/77186) (CWG2823, open)
**Affects:** measured 2026-10-13 -- 18.1.0, 22.1.6, the stock trunk nightly
`33891afe2ebb` (with and without
`-fexperimental-new-constant-interpreter`) and this branch's install.  GCC the
same (gnu_gcc GCC-749).
**D4277R1 category:** `ub:expr.unary.dereference.nullptr (.indirection)`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-null-static-member-call.cpp` (XFAIL)

## Reproducer

[`clang-737-constexpr-null-static-member-call.cpp`](clang-737-constexpr-null-static-member-call.cpp),
at `-std=c++26 -fsyntax-only`:

    struct S { static constexpr int sf() { return 1; } static constexpr int k = 1; };
    constexpr bool f() {
      S* sp = nullptr;
      sp->sf();       // undefined: evaluates *sp, an indirection through a null pointer
      return true;
    }
    static_assert(f());
    constexpr bool g() {
      S* sp = nullptr;
      return sp->k == 1;   // the same, for a static data member
    }
    static_assert(g());

Accepted.  `E1->E2` is `(*E1).E2` ([expr.ref]), and the object expression
is evaluated even when the member is static, so both evaluate `*sp`, an
indirection through a null pointer, which is undefined ([expr.unary.op]/1
as amended by CWG2823); neither static_assert's condition is a core
constant expression.  Since Clang 22 the explicit form `(*sp).sf()`, a
discarded `*p` and `&*p` are rejected ("dereferencing a null pointer is
not allowed in a constant expression"); Clang 18 to 21 accepted those too.

## Cause

`LValueExprEvaluator::VisitMemberExpr` (`clang/lib/AST/ExprConstant.cpp`)
handles a static data member or static member function by calling
`VisitIgnoredBaseExpression(E->getBase())` and returning the member.  For
`->` the base is the pointer prvalue, which is evaluated as an ignored
value; the implied indirection `*sp` is never formed as an lvalue, so the
null check that rejects `(*sp).sf()` never runs.  A fix would, for an arrow
member access, check that the evaluated pointer designates an object (null
and, by the same reasoning, past-the-end) before discarding it.

## Upstream search (2026-10-13)

GitHub issue search on llvm/llvm-project including closed issues, control
validated (`CWG2823` returned #77186).  Queries: `constexpr static member
function null pointer`; `CWG2823 static member`; `static member call through
null pointer constant expression`; `constexpr null pointer arrow static`;
`CWG2823`; `"null pointer" constexpr "static member"`; `dereferencing null
pointer constexpr static member function`; `null pointer static member
function constant evaluation`; `static data member null pointer constexpr`;
`constexpr arrow null static`; `nullptr static member access constant
expression`; `VisitIgnoredBaseExpression`.  #77186 is the CWG2823 umbrella
(reference binding to `*nullptr`); none is this form.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** A static member named through `->` on a null pointer is
accepted in constant evaluation

**Description:** `sp->sf()` and `sp->k`, with `sp` a null pointer and the
members static, evaluate `*sp` ([expr.ref]), an indirection through a null
pointer, which is undefined ([expr.unary.op]/1, CWG2823), so neither
static_assert's condition is a core constant expression.  Both are accepted
(by either constant evaluator); `(*sp).sf()` is rejected.  Related #77186.

    struct S { static constexpr int sf() { return 1; } static constexpr int k = 1; };
    constexpr bool f() { S* sp = nullptr; sp->sf(); return true; }
    static_assert(f());
    constexpr bool g() { S* sp = nullptr; return sp->k == 1; }
    static_assert(g());

## Found by

The M53-Clang mirror (CLANG-736) of gnu_gcc GCC-749's watch test, whose
static member function call through a null pointer Clang accepts too; the
static data member form was found while reducing it.
