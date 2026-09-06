# CLANG-14: a pure virtual's dependent contract predicate reaches CodeGen

**Status:** Open (defect)
**Kind:** defect
**Component:** CodeGen / Sema (contracts, P3097)
**Upstream Link:** `--` (P3097 virtual contracts are not upstream in Clang;
nothing to file)
**Found:** 2026-09-05, while probing for gaps after CLANG-13 -- specifically by
asking whether a pure virtual's interface contract, which GCC-34's fix had just
made *exist* on the GCC side, actually **fires** at run time.

## Symptom

```
clang/lib/AST/ExprConstant.cpp:22215:
  bool clang::Expr::EvaluateAsInt(...):
  Assertion `!isValueDependent() &&
    "Expression evaluator can't be called on a dependent expression."' failed.
```

## Reproducer

[`clang-14-pure-virtual-dependent-predicate-codegen.cpp`](clang-14-pure-virtual-dependent-predicate-codegen.cpp):

```c++
template <class T> struct A {
  virtual int get() const pre(n >= 0) = 0;
  int n;
};
struct D : A<int> { int get() const override { return n; } };
int main() { D d; A<int> &a = d; return a.get(); }
```

```
clang++ -std=c++26 -fcontracts -fcontracts-p3097 -c clang-14-...cpp
```

## What narrows it

Measured, one variable at a time:

| | result |
|---|---|
| pure virtual, class template, `pre(n >= 0)` | **crash** |
| pure virtual, class template, `pre(sizeof(T) == 4)` | **crash** |
| pure virtual, class template, `pre(f() == 0)` | OK |
| pure virtual, class template, `pre(true)` / `pre(1 == 0)` | OK |
| pure virtual, class template, `post(r: r >= 0)` | OK |
| **non-pure** virtual, class template, `pre(n >= 0)` | OK |
| pure virtual, **non-template** class, `pre(n >= 0)` | OK |

So all three of *pure*, *class template*, and a predicate that is
**value-dependent when parsed** are required. `f() == 0` is a call but not
dependent, and it passes -- which is what separates this from GCC-34, where a
call was enough.

**`-fsyntax-only` does not reproduce it.** The crash is in CodeGen. The
28-shape probe matrix that found CLANG-13 was entirely `-fsyntax-only` and
walked straight past this; it only surfaced when a *runnable* test tried to
prove a pure virtual's interface contract actually fires.

## Provenance: pre-existing, not the CLANG-13 fix

Measured rather than argued. The pre-CLANG-13-fix sources
(`b9bdd3345c6a^` for `SemaTemplateInstantiateDecl.cpp` and `Sema.h`) were put
back in the working tree, rebuilt in the Space, and given the same reproducer:
**identical assertion**. So the CLANG-13 scope fix and cycle guard neither
caused nor masked it.

## GCC

**Not affected.** GCC compiles all of the crashing shapes, and a runnable
version confirms the pure virtual's interface precondition is genuinely
evaluated through the P3097 wrapper (violation count 1 for a failing value, 0
for a passing one). Note this is only true *after* GCC-34 was fixed the same
day -- before that GCC ICEd on an overlapping but not identical set. The two
compilers were each broken here, in different halves, and the intersection
(`pure + class template + dependent predicate`) is what nobody had a test for.

Mirrored watch tests, opposite expectations as usual:

* `clang/test/Contracts/OpenBugs/pure-virtual-dependent-predicate-codegen.cpp`
  -- `XFAIL: *`.
* `gcc/testsuite/g++.dg/contracts/cpp26/open-bug-pure-virtual-dependent-predicate-codegen.C`
  -- expected pass.

## Impact

Any P3097 pure virtual in a class template whose predicate mentions a member
or a template parameter. Not reached by BDE today (its `dbg_64_cpp26_contracts`
build is clean on both compilers), and not reached by the contracts test suite,
which is why it survived this long.
