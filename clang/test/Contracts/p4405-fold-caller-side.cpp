// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-fold-caller-side.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405 stage 2: a caller-side evaluation of a precondition is folded with
// the call's arguments substituted for the parameters, when the argument
// folds to a constant and the parameter is passed by value and has scalar
// type.  Every other case -- a reference parameter, a non-constant
// argument -- keeps the parameter unknown, and so stays unproven unless the
// predicate holds whatever its value.  The callee-side evaluations are
// ignore, so the definitions are not diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-caller-side.C.
// One call differs: Clang does not fold d == d for an unknown d, so
// either(n, m) is unproven.  A constructor of a class with a virtual base
// (no in-charge parameter in Clang's AST) folds as in gnu_gcc since its
// GCC-917: W w(1) is proven and W w0(0) always false.  The calls
// through a pointer, on a static object and of an inheriting constructor
// are in gnu_gcc's mirror too (since GCC-888's fix).

int divide(int n, int d) pre(d != 0);   // expected-note 4 {{the predicate of the contract assertion}}
int scale(int x, int f = 0) pre(f != 0); // expected-note {{the predicate of the contract assertion}}
int scale3(int x, int f = 3) pre(f > 0);
int byref(const int &d) pre(d != 0);    // expected-note {{the predicate of the contract assertion}}
int deref(int *p) pre(p != nullptr);    // expected-note {{the predicate of the contract assertion}}
int va(int n, ...) pre(n > 0);
int either(int n, int d) pre(d == d);   // expected-note {{the predicate of the contract assertion}} \
                                        // expected-warning {{self-comparison always evaluates to true}}
enum class E { a, b };
int pick(E e) pre(e == E::b);           // expected-note {{the predicate of the contract assertion}}
int k();
int global;
constexpr int two = 2;

struct S {
  S(int x) pre(x > 0);                  // expected-note 2 {{the predicate of the contract assertion}}
  void m(int x) pre(x > 0);             // expected-note {{the predicate of the contract assertion}}
  void self(const S *p) pre(p == this); // expected-note {{the predicate of the contract assertion}}
};
S gs(1);
struct T : S {
  using S::S;
};

struct V {};
struct W : virtual V {
  W(int x) pre(x > 0);                  // expected-note {{the predicate of the contract assertion}}
};

template <class... T>
void all_positive(T... t) pre(((t > 0) && ...)); // expected-note {{the predicate of the contract assertion}}

void use(int n, int m, S *ps) {
  divide(n, 0);         // expected-error {{contract predicate is always false}}
  divide(n, 2);         // proven
  divide(n, two);       // proven
  divide(n, 1 + 1);     // proven
  divide(n, m);         // expected-error {{contract assertion cannot be proven}}
  divide(n, k());       // expected-error {{contract assertion cannot be proven}}
  divide(n, (k(), 2));  // expected-error {{contract assertion cannot be proven}}
  either(n, m);         // expected-error {{contract assertion cannot be proven}}
  scale(n);             // expected-error {{contract predicate is always false}}
  scale3(n);            // proven
  byref(0);             // expected-error {{contract assertion cannot be proven}}
  deref(nullptr);       // expected-error {{contract predicate is always false}}
  deref(&global);       // proven
  va(1, 2, 3);          // proven
  pick(E::b);           // proven
  pick(E::a);           // expected-error {{contract predicate is always false}}
  S s(1);               // proven
  S t(0);               // expected-error {{contract predicate is always false}}
  T u(1);               // proven: the inherited constructor's arguments
  T v(0);               // expected-error {{contract predicate is always false}}
  s.m(2);               // proven
  s.m(0);               // expected-error {{contract predicate is always false}}
  ps->m(3);             // proven
  gs.self(&gs);         // proven: `this` is the static object gs
  ps->self(&gs);        // expected-error {{contract assertion cannot be proven}}
  W w(1);               // proven
  W w0(0);              // expected-error {{contract predicate is always false}}
  all_positive(1, 2);   // proven
  all_positive(1, 0);   // expected-error {{contract predicate is always false}}
  decltype(divide(n, 0)) z = 0;
}
