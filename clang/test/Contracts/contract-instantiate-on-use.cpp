// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fsyntax-only -verify %s
// expected-no-diagnostics

// A template's contracts are substituted when it is odr-used, not only when
// its definition is instantiated.
//
// This is what InstantiateFunctionContractsOnUse is for, and Clang has always
// got it right; the file exists because GCC did not.  GCC deferred contract
// substitution to instantiate_body, so a function that is never defined never
// had its contracts substituted at all, and its P3097 interface wrapper walked
// the still-dependent predicate into an ICE.  That was GCC-34, fixed
// 2026-09-05 by giving GCC the same on-odr-use instantiation.  This side
// pins the behaviour so the two suites ask the same question.
//
// `pre(true)` would not be a regression test for it: a predicate that is
// already a constant needs no substitution to be usable, which is why the
// defect hid for as long as it did.  Every predicate below contains something
// substitution has to rewrite.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-instantiate-on-use.C.  That file
// carries two further cases this one cannot: P3595 caller-side checking of a
// declared-only function template, and of a declared-only member of a class
// template.  Clang accepts caller-side configuration but does not implement
// caller-side checking (see p3595-caller-unimplemented-warn.cpp), so nothing
// here ever reads a declared-only template's contracts and the shape is not
// constructible.

int f();

//===----------------------------------------------------------------------===//
// A pure virtual in a class template, predicate containing a call.  Its
// definition is never instantiated, so on-use instantiation is the only thing
// that will ever substitute its interface contracts.
//===----------------------------------------------------------------------===//

template <class T> struct PureCall {
  virtual void g() pre(f() == 0) = 0;
};

struct ConcreteCall : PureCall<int> {
  void g() override {}
};

void use_pure_call(PureCall<int> &a) { a.g(); }

//===----------------------------------------------------------------------===//
// The same with a type-dependent predicate, which on GCC reached a different
// assertion -- so a fix that only quietened the first one would have left this
// standing.
//===----------------------------------------------------------------------===//

template <class T> struct PureDependent {
  virtual void g() pre(sizeof(T) == 4) = 0;
};

struct ConcreteDependent : PureDependent<int> {
  void g() override {}
};

void use_pure_dependent(PureDependent<int> &a) { a.g(); }

//===----------------------------------------------------------------------===//
// A pure virtual whose postcondition captures (P3098): the capture list has to
// substitute too, not just the predicate.
//===----------------------------------------------------------------------===//

template <class T> struct PureCapturing {
  virtual int get() const post [old = f()] (old == f()) = 0;
};

struct ConcreteCapturing : PureCapturing<int> {
  int get() const override post [old = f()] (old == f()) { return 0; }
};

void use_pure_capturing(PureCapturing<int> &a) { a.get(); }

//===----------------------------------------------------------------------===//
// Control: a predicate that is already a constant needs no substitution.  Kept
// so that narrowing the behaviour to "only when dependent" still leaves this
// passing rather than silently changing which cases are covered.
//===----------------------------------------------------------------------===//

template <class T> struct PureConstant {
  virtual void g() pre(true) = 0;
};

struct ConcreteConstant : PureConstant<int> {
  void g() override {}
};

void use_pure_constant(PureConstant<int> &a) { a.g(); }

//===----------------------------------------------------------------------===//
// Control: a virtual that IS defined.  Its contracts are substituted when the
// definition instantiates, so the on-use pass must be a no-op rather than
// substituting a second time.
//===----------------------------------------------------------------------===//

template <class T> struct Defined {
  virtual void g() pre(f() == 0) {}
};

void use_defined(Defined<int> &d) { d.g(); }
