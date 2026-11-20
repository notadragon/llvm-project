# CLANG-779: An empty class copied from one past the end of an array is accepted in constant evaluation

**Status:** Open here; open upstream.  Deferral proposed: notadragon_wg21
DECISIONS.md E104
**Component:** clang:frontend (constexpr)
**Upstream Link:** None found (searched 2026-10-16) -- filing owed
**Affects:** measured 2026-10-16 -- 18.1.0, 19.1.0, 20.1.0, 21.1.0, 22.1.6,
the stock trunk nightly `33891afe2ebb` and this branch's install
(`4d44363fadf3`).  The trunk's `-fexperimental-new-constant-interpreter`
rejects all three.  GCC rejects the copy and the move and accepts the
assignment (stock 14.4 to trunk; gnu_gcc GCC-831, with its empty store
past the end).
**D4277R1 category:** `ub:expr.unary.dereference.invalid.known`
**Watch test:** `clang/test/Contracts/OpenBugs/constexpr-empty-copy-past-end.cpp`
(XFAIL)

## Reproducer

[`clang-779-constexpr-empty-copy-past-end.cpp`](clang-779-constexpr-empty-copy-past-end.cpp),
at `-std=c++26 -fsyntax-only`:

    struct E {};
    constexpr bool copy(int i) { E a[2]; E e2 = a[i]; (void)e2; return true; }
    static_assert(copy(2));
    constexpr bool move(int i) { E a[2]; E e2 = static_cast<E &&>(a[i]); (void)e2; return true; }
    static_assert(move(2));
    constexpr bool from(int i) { E a[2]; E e; e = a[i]; return true; }
    static_assert(from(2));

All three are accepted.  `a[2]` is `*(a + 2)`, an indirection through the
past-the-end pointer, which points to no object; [expr.unary.op]/1 makes
that undefined, so none of the calls is a core constant expression
([expr.const]).  The same copy of a non-empty class, and a store of an
empty class to the element (`a[i] = e`, "member call on dereferenced
one-past-the-end pointer"), are rejected.

## Cause

The trivial copy constructor and copy assignment of a class that an
lvalue-to-rvalue conversion would not read
(`isReadByLvalueToRvalueConversion` in `clang/lib/AST/ExprConstant.cpp`
returns false for an empty class) are evaluated without reading the
source, so the source lvalue -- designating one past the end -- is never
checked against the object it should designate.  A fix would check the
source designates an object (as `findCompleteObject` does for a read)
even when nothing is read.

## Consequence on this branch

None for the P3100 checks: DECISIONS.md E95 (a) makes these copies
`ub:expr.unary.dereference.invalid.known` run-time checks, which constant
evaluation does not use.  A variable whose initializer is manifestly
constant-evaluated and calls such a function is constant-initialized, so
the run-time check inside it never runs.

## Upstream search (2026-10-16)

GitHub search on llvm/llvm-project, issues and pull requests, including
closed ones.  Queries: `is:issue constexpr empty class one-past-the-end
copy`; `is:issue constant expression empty struct past the end copy
accepted`; `is:issue one-past-the-end empty class constexpr`; `is:issue
trivial copy empty class past-the-end constant`; `constexpr empty
one-past-the-end`; `constexpr "empty class" "past the end"`; `constexpr
copy "one past the end"`.  None matches.

## Draft upstream report (LLVM GitHub; filing owed -- the user files)

**Summary:** Constant evaluation accepts a copy of an empty class from one
past the end of an array

    struct E {};
    constexpr bool copy(int i) { E a[2]; E e2 = a[i]; (void)e2; return true; }
    static_assert(copy(2));   // accepted; should be rejected

`a[2]` designates no object ([expr.unary.op]/1: "the result is an lvalue
referring to the object ... to which the expression points"; there is
none one past the end), so the evaluation has undefined behaviour and the
call is not a core constant expression.  Clang rejects the copy of a
non-empty class from `a[2]` ("read of dereferenced one-past-the-end
pointer") and the store `a[2] = e` ("member call on dereferenced
one-past-the-end pointer"), but the trivial copy of an empty class --
constructed, moved, or assigned from (`e = a[i]`) -- reads nothing, so the
designated object is never checked.  Clang 18 through trunk; the
experimental constant interpreter rejects it.  GCC rejects the copy and
the move.
