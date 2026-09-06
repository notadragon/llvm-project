// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -emit-llvm -o /dev/null -verify %s
// expected-no-diagnostics

// A pure virtual in a class template whose contract predicate is
// value-dependent when parsed.  This was CLANG-14 until 2026-09-06: the
// predicate reached CodeGen still dependent and the constant evaluator
// asserted on `!isValueDependent()`.
//
// The cause was the trigger, not the substitution.  A pure virtual called by
// unqualified virtual dispatch is deliberately NOT odr-used --
// [basic.def.odr] says a function is odr-used when "named by a potentially
// evaluated expression" and excludes a pure virtual from being named by one
// unless explicitly qualified -- so MarkMemberReferenced passes
// MightBeOdrUse = false, and contract instantiation, which hung off
// `OdrUse == Used`, never ran.  Nothing else would ever substitute them
// either: a pure virtual has no definition to instantiate.
//
// [dcl.contract.func] makes contracts needed only when the function "is
// odr-used or the function is defined", so as written it never makes a pure
// virtual's contracts needed at all -- a core issue is being filed to add a
// bullet matching [except.spec]'s "in an expression, the function is selected
// by overload resolution".  [except.spec] already has that bullet, which is
// why the exception specification of the same pure virtual was always
// resolved at the same call while its contracts were not.
//
// THE RUN LINE MUST STAY A CODEGEN ONE.  -fsyntax-only passed throughout,
// which is how a 28-shape syntax-only probe matrix walked straight past this.
//
// The controls below are load-bearing: a non-dependent call predicate, a
// constant predicate, a non-pure virtual and a non-template class all worked
// before the fix.  If the whole file ever fails, they say the cause has moved
// rather than that this specific bug is back.
//
// GCC mirror: g++.dg/contracts/cpp26/pure-virtual-dependent-predicate-codegen.C

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
