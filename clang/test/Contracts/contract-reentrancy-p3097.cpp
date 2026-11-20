// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s
// expected-no-diagnostics

// Re-entrant contract instantiation crossing P3097 virtual functions.
//
// A virtual function raises the stakes twice over the plain case
// (contract-reentrancy-basic.cpp).  Its interface contracts are evaluated by a
// wrapper around the vtable dispatch, so they must exist as a substituted,
// non-dependent specifier even when the function's own definition is never
// instantiated -- which is precisely why InstantiateFunctionContractsOnUse
// runs at every odr-use, and therefore why it can be re-entered from inside
// another contract's predicate.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-reentrancy-p3097.C.

//===----------------------------------------------------------------------===//
// The predicate odr-uses a contracted virtual member of a class template, so
// the interface-contract wrapper's contracts are instantiated re-entrantly.
//===----------------------------------------------------------------------===//

template <class T> struct S {
  virtual bool ok() const pre(n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T> int f_calls_virtual(S<T> &s) pre(s.ok()) { return s.n; }

int use_calls_virtual() {
  S<int> s;
  return f_calls_virtual(s);
}

//===----------------------------------------------------------------------===//
// The OUTER contract is the one on a virtual function: the nested pass starts
// from inside the wrapper's own interface contract.
//===----------------------------------------------------------------------===//

template <class T> struct H {
  bool ok() const pre(n >= 0) { return n >= 0; }
  int n = 0;
};

template <class T> struct OuterVirtual {
  virtual int f(H<T> h) pre(h.ok()) { return h.n; }
};

int use_outer_virtual() {
  OuterVirtual<int> s;
  return s.f(H<int>{});
}

//===----------------------------------------------------------------------===//
// Both ends virtual: a virtual function's precondition dispatches to another
// virtual function that has contracts of its own.
//===----------------------------------------------------------------------===//

template <class T> struct BothVirtual {
  virtual bool ok() const pre(n >= 0) { return n >= 0; }
  virtual int get() const pre(ok()) { return n; }
  int n = 0;
};

int use_both_virtual() {
  BothVirtual<int> b;
  return b.get();
}

//===----------------------------------------------------------------------===//
// The predicate dispatches through a base reference and the override carries
// its own contracts -- P3097 gives each override independent contracts, so
// both the base's and the derived's must substitute.
//===----------------------------------------------------------------------===//

template <class T> struct B {
  virtual bool ok() const pre(true) { return true; }
};

template <class T> struct D : B<T> {
  bool ok() const override pre(true) { return true; }
};

template <class T> int f_override(B<T> &b) pre(b.ok()) { return 0; }

int use_override() {
  D<int> d;
  B<int> &b = d;
  return f_override(b);
}

//===----------------------------------------------------------------------===//
// The predicate odr-uses a PURE virtual with contracts.  Its own definition is
// never instantiated, so the on-use pass is the only thing that will ever
// substitute its interface contracts.
//===----------------------------------------------------------------------===//

template <class T> struct A {
  virtual bool ok() const pre(true) = 0;
};

template <class T> int f_pure(A<T> &a) pre(a.ok()) { return 0; }

struct Concrete : A<int> {
  bool ok() const override pre(true) { return true; }
};

int use_pure() {
  Concrete c;
  A<int> &a = c;
  return f_pure(a);
}
