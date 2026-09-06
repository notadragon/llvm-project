// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -emit-llvm -o /dev/null %s
// XFAIL: *

// OPEN BUG (CLANG-14 in this fork's open-issues/): a pure virtual in a class
// template whose contract predicate is value-dependent when parsed reaches
// CodeGen still dependent, and the constant evaluator asserts:
//
//   clang/lib/AST/ExprConstant.cpp: Expr::EvaluateAsInt(...):
//   Assertion `!isValueDependent() &&
//     "Expression evaluator can't be called on a dependent expression."'
//
// All three of pure, class template, and a *value-dependent* predicate are
// required.  `pre(f() == 0)` is a call but not dependent and compiles clean;
// so do `pre(true)` and the non-pure and non-template variants.  Those
// negatives are what separate this from GCC-34, where a call was enough.
//
// NOTE THE RUN LINE: -emit-llvm, not -fsyntax-only.  The crash is in CodeGen,
// and a syntax-only RUN line passes -- which is exactly how the 28-shape probe
// matrix behind CLANG-13 missed it.  It surfaced only when a runnable test
// asked whether a pure virtual's interface contract actually fires.
//
// Pre-existing, not the CLANG-13 fix: measured by rebuilding
// SemaTemplateInstantiateDecl.cpp and Sema.h at b9bdd3345c6a^, which asserts
// identically.
//
// THIS ROW IS CLANG-ONLY.  GCC compiles it, and a runnable version confirms
// the interface precondition is genuinely evaluated through the P3097 wrapper,
// so the gnu_gcc mirror expects success rather than xfailing.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-pure-virtual-dependent-predicate-codegen.C

int f();

// THE OPEN BUG: `n` is a member of a dependent class, so the predicate is
// value-dependent when parsed.
template <class T> struct A {
  virtual int get() const pre(n >= 0) = 0;
  int n;
};

struct D : A<int> {
  int get() const override { return n; }
};

int use_dependent_member() {
  D d;
  A<int> &a = d;
  return a.get();
}

// Also dependent, by way of the template parameter rather than a member.
template <class T> struct B {
  virtual int get() const pre(sizeof(T) == 4) = 0;
};

struct E : B<int> {
  int get() const override { return 0; }
};

int use_dependent_param() {
  E e;
  B<int> &b = e;
  return b.get();
}

// Controls, all fine today: a non-dependent call, a constant, a non-pure
// virtual, and a non-template class.  They keep the failure attributable --
// if the whole file started failing, these would say the cause had moved.

template <class T> struct NotDependent {
  virtual int get() const pre(f() == 0) = 0;
};
struct F1 : NotDependent<int> { int get() const override { return 0; } };
int use_not_dependent() { F1 x; NotDependent<int> &r = x; return r.get(); }

template <class T> struct NotPure {
  virtual int get() const pre(n >= 0) { return n; }
  int n;
};
int use_not_pure() { NotPure<int> x{}; return x.get(); }

struct NotTemplate {
  virtual int get() const pre(n >= 0) = 0;
  int n;
};
struct F2 : NotTemplate { int get() const override { return n; } };
int use_not_template() { F2 x{}; NotTemplate &r = x; return r.get(); }
