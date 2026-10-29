// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -Wno-tautological-undefined-compare %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): a constructor precondition that names
// `this', analyzed at a call that makes a prvalue (`T(y)'), is given the
// object the prvalue initializes -- the temporary, or the object an
// initialization that elides the temporary initializes instead: a
// variable, a returned object (the indirect return slot), an aggregate's
// member, an array element, a new-expression's object, a by-value
// parameter, a base subobject, either arm of a conditional, in a lambda.
// Each guarded call is proven; an unguarded one is not, nor one whose guard
// the proof cannot see (a lambda's capture; the argument of a base's
// constructor in a mem-initializer).  (CLANG-857: the returned forms were
// not proven, the return slot's pointer unknown to the proof shadow.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-this-elide.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct T {
  T(int y) pre<always_prove>(this != nullptr && y > 0); // expected-note 3 {{the predicate of the contract assertion}}
  T(const T &);
  ~T();
  int v = 0;
};
struct A { T t; int k; };
struct D : T { D(int y) : T(T(y)) {} }; // expected-error {{contract assertion cannot be proven}}
void take(T);

T ret(int y) {
  if (y > 0)
    return T(y);
  return T(1);
}

T nrvo() {
  T t(1);
  return t;
}

int var(int y) {
  if (y > 0) {
    T x = T(y);
    return x.v;
  }
  return 0;
}

int agg(int y) {
  if (y > 0) {
    A a{T(y), 1};
    return a.k;
  }
  return 0;
}

int arr(int y) {
  if (y > 0) {
    T a[] = {T(y), T(y)};
    return a[0].v;
  }
  return 0;
}

T *nw(int y) {
  if (y > 0)
    return new T(T(y));
  return nullptr;
}

void arg(int y) {
  if (y > 0)
    take(T(y));
}

int base(int y) {
  if (y > 0) {
    D d(y);
    return d.v;
  }
  return 0;
}

int cond(int y, bool b) {
  if (y > 0) {
    T x = b ? T(y) : T(y);
    return x.v;
  }
  return 0;
}

int lam(int y) {
  if (y > 0)
    return [&] { return T(y).v; }(); // expected-error {{contract assertion cannot be proven}}
  return 0;
}

int unguarded(int y) {
  T x = T(y); // expected-error {{contract assertion cannot be proven}}
  return x.v;
}
