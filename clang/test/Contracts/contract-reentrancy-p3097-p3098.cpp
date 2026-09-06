// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fsyntax-only -verify %s
// expected-no-diagnostics

// Re-entrant contract instantiation with P3097 virtual functions and P3098
// postcondition captures at the same time.
//
// These two extensions are the ones that fundamentally alter the control flow
// around a contract check: P3097 evaluates interface contracts in a wrapper
// around the vtable dispatch, and P3098 splits a postcondition into an
// on-entry capture and an on-exit predicate.  A virtual function with a
// capturing postcondition does both, and reaching one re-entrantly -- from
// inside another contract's predicate, before that predicate has finished
// transforming -- is the densest combination the feature set allows.
//
// See contract-reentrancy-basic.cpp for the shape of the underlying defect,
// and the -p3097 / -p3098 files for each extension on its own.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-reentrancy-p3097-p3098.C.

//===----------------------------------------------------------------------===//
// The predicate odr-uses a virtual function that has a capturing
// postcondition: the wrapper and the capture list must both substitute during
// the nested pass.
//===----------------------------------------------------------------------===//

template <class T> struct S {
  virtual int get() const post [old = n] (old == n) { return n; }
  int n = 0;
};

template <class T> int f_virtual_capturing(S<T> &s) pre(s.get() >= 0) {
  return s.n;
}

int use_virtual_capturing() {
  S<int> s;
  return f_virtual_capturing(s);
}

//===----------------------------------------------------------------------===//
// Virtual with a capturing postcondition at BOTH ends, and the odr-use in the
// outer capture initializer rather than the outer predicate.
//===----------------------------------------------------------------------===//

template <class T> struct H {
  virtual int get() const post [was = n] (was == n) { return n; }
  int n = 0;
};

template <class T> struct BothEnds {
  virtual int f(H<T> &h) post [old = h.get()] (old >= 0) { return h.n; }
};

int use_both_ends() {
  BothEnds<int> s;
  H<int> h;
  return s.f(h);
}

//===----------------------------------------------------------------------===//
// An override with its own capturing postcondition, reached virtually from a
// predicate.  P3097 gives the override independent contracts, so the base's
// and the derived's capture lists are separate substitutions.
//===----------------------------------------------------------------------===//

template <class T> struct B {
  virtual int get() const post [old = m] (old == m) { return m; }
  int m = 0;
};

template <class T> struct D : B<T> {
  int get() const override post [old = this->m] (old == this->m) {
    return this->m;
  }
};

template <class T> int f_override(B<T> &b) pre(b.get() >= 0) { return 0; }

int use_override() {
  D<int> d;
  B<int> &b = d;
  return f_override(b);
}

//===----------------------------------------------------------------------===//
// A pure virtual with a capturing postcondition: nothing but the on-use pass
// will ever substitute its interface contracts, and it is reached from inside
// another contract's predicate.
//
// The GCC mirror does NOT carry this case: GCC substitutes contracts only when
// it instantiates a definition, which for a pure virtual never happens, so its
// P3097 wrapper ICEs on the dependent predicate.  That is GCC-34, tracked in
// the gnu_gcc fork's open-issues/ and xfailed there as
// open-bug-gcc34-pure-virtual-wrapper.C.  Clang is fine precisely because
// InstantiateFunctionContractsOnUse exists -- the same hook that produced the
// re-entrancy defect this whole file is about.
//===----------------------------------------------------------------------===//

template <class T> struct A {
  virtual int get() const post [old = tag()] (old == tag()) = 0;
  static int tag() { return 0; }
};

template <class T> int f_pure(A<T> &a) pre(a.get() >= 0) { return 0; }

struct Concrete : A<int> {
  int get() const override post [old = tag()] (old == tag()) { return 0; }
};

int use_pure() {
  Concrete c;
  A<int> &a = c;
  return f_pure(a);
}
