# CLANG-756: An indirection through a null pointer to function is accepted in constant evaluation

**Status:** Open here; open upstream.  Deferred by the user 2026-10-14
(notadragon_wg21 DECISIONS.md E88)
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-14) -- filing owed; related
[#77186](https://github.com/llvm/llvm-project/issues/77186) (CWG2823, open)
and [#69108](https://github.com/llvm/llvm-project/issues/69108) (open; the
object form, rejected since PR #143667)
**Affects:** measured 2026-10-14 -- 18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6,
the stock trunk nightly `33891afe2ebb` (with and without
`-fexperimental-new-constant-interpreter`) and this branch's install.  The
object forms (`S &r = *p`, `&*p`) are rejected since 22.1.6.  GCC rejects
the reference binding and accepts the other two (gnu_gcc GCC-749, deferred;
its watch test `open-bug-constexpr-null-indirection-no-read.C`).
**D4277R1 category:** `ub:expr.unary.dereference.nullptr (.indirection)`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-null-function-indirection.cpp`
(XFAIL)

## Reproducer

[`clang-756-constexpr-null-function-indirection.cpp`](clang-756-constexpr-null-function-indirection.cpp),
at `-std=c++26 -fsyntax-only`:

    using F = int();
    constexpr int bind(F *fp) { F &r = *fp; (void)r; return 1; }
    static_assert(bind(nullptr) == 1);
    constexpr bool addr(F *fp) { return &*fp == nullptr; }
    static_assert(addr(nullptr));
    constexpr int disc(F *fp) { *fp; return 1; }
    static_assert(disc(nullptr) == 1);

All three are accepted.  `*fp` with `fp` a null pointer is an indirection
through a null pointer, undefined by [expr.unary.op]/1 for a function as for
an object ("If the operand points to an object or function, the result
denotes that object or function; otherwise, the behavior is undefined"), so
none of the calls is a core constant expression ([expr.const]).  The same
three forms through a null pointer to an object are rejected; calling
through the null pointer, `(*fp)()`, is rejected.

## Cause

`LValueExprEvaluator::VisitUnaryDeref` (`clang/lib/AST/ExprConstant.cpp`)
checks the designated object only for an object type:

    if (!Success || !E->getType().getNonReferenceType()->isObjectType())
      return Success;
    return bool(findCompleteObject(Info, E, AK_Dereference, Result,
                                   E->getType())) ||
           Info.noteUndefinedBehavior();

For a function type the null lvalue is returned as a success, and nothing
later looks at it unless it is called.  A fix would reject a null base for
a function type there (as the call does).

## Consequence on this branch

Because the evaluation succeeds, a variable whose initializer is manifestly
constant-evaluated and calls such a function -- `const int r =
bind(nullptr);`, or a static `int s = bind(nullptr);` -- is
constant-initialized, and the P3100 `.indirection` check of the binding
inside `bind` never runs.  A call that is not manifestly constant-evaluated
(`int r = bind(nullptr);`) is emitted as a call and checked.

## Upstream search (2026-10-14)

GitHub search on llvm/llvm-project, issues and pull requests, including
closed ones.  Queries: `CWG2823` (found #77186, PR #143667 "Diagnose forming
references to nullptr", the object fix, and #165633, tests); `is:issue
constexpr dereference null function pointer` (found #69108, the object
reference); `is:issue constant expression null function pointer
indirection`; `is:issue constexpr "function pointer" null dereference
accepted`; `is:issue constexpr reference bound to null function`.  None
covers a pointer to function.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Constant evaluation accepts `*fp` for a null pointer to
function

**Description:**

    using F = int();
    constexpr int bind(F *fp) { F &r = *fp; (void)r; return 1; }
    static_assert(bind(nullptr) == 1);   // accepted
    constexpr bool addr(F *fp) { return &*fp == nullptr; }
    static_assert(addr(nullptr));        // accepted
    constexpr int disc(F *fp) { *fp; return 1; }
    static_assert(disc(nullptr) == 1);   // accepted

[expr.unary.op]/1 makes the indirection undefined when the operand points to
no object or function, so none of these is a constant expression.  The same
forms with a pointer to an object are rejected since #143667;
`LValueExprEvaluator::VisitUnaryDeref` returns early for a non-object type
before checking the pointer.  Related: #77186 (CWG2823), #69108.  GCC
rejects the reference binding.

## Found by

M63-Clang (2026-10-14), measuring the constant-evaluation side of
CLANG-755 (a call or reference binding through a null pointer to function
not checked at run time; fixed here).
