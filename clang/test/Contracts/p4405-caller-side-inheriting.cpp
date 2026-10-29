// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-fold-caller-side.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405: a call of an inheriting constructor evaluates the inherited
// constructor's caller-side evaluations, so their prove and diagnose are
// diagnosed at each call, folded with its arguments, and never at the
// using-declaration: through two levels of inheriting constructors, from a
// virtual base, from a class template's specialization, from a constructor
// template (declared only; one with a definition is in
// p4405-template-caller.cpp), in a new-expression, and not in an
// unevaluated operand.  Each diagnostic has its note at the predicate.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-caller-side-inheriting.C
// (gnu_gcc, P5-GCC).

struct S {
  S(int x) pre(x > 0);          // expected-note 6 {{the predicate of the contract assertion}}
};
struct T : S {
  using S::S;
};
struct T2 : T {
  using T::T;
};
struct TV : virtual S {
  using S::S;
};

template <class X>
struct SX {
  SX(X x) pre(x > 0);           // expected-note {{the predicate of the contract assertion}}
};
struct TX : SX<int> {
  using SX<int>::SX;
};

struct ST {
  template <class U>
  ST(U u) pre(u > 0);           // expected-note 2 {{the predicate of the contract assertion}}
};
struct TT : ST {
  using ST::ST;
};

void use(int n) {
  T a(n);                       // expected-error {{contract assertion cannot be proven}}
  T b(n);                       // expected-error {{contract assertion cannot be proven}}
  T c(2);                       // proven
  T2 d(n);                      // expected-error {{contract assertion cannot be proven}}
  T2 e(0);                      // expected-error {{contract predicate is always false}}
  T2 f(3);                      // proven
  TV g(n);                      // expected-error {{contract assertion cannot be proven}}
  TV h(1);                      // proven
  TX i(0);                      // expected-error {{contract predicate is always false}}
  TX j(4);                      // proven
  TT k(n);                      // expected-error {{contract assertion cannot be proven}}
  TT l(0);                      // expected-error {{contract predicate is always false}}
  TT m(5);                      // proven
  T *p = new T(0);              // expected-error {{contract predicate is always false}}
  decltype(T(0)) *q = p;        // not evaluated
  (void)q;
}
