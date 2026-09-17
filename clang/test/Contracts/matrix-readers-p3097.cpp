// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s
// GENERATED from a contract-feature cross-product specification.
// Do not edit by hand -- regeneration discards any change made here.
//
// Who needs a function's contracts, crossed with what state they are in and what the predicate contains.
//
// Three axes.  READER: the function's own definition, a P3097
// interface wrapper, a P3595 caller-side check, constant evaluation, or another
// contract's predicate.  PRODUCER: defined, declared-only, pure virtual,
// out-of-line, explicitly instantiated.  PREDICATE KIND: a constant, a
// non-dependent call, a type-dependent expression, or a reference to a member of
// the dependent class.
//
// The third axis is the one worth defending.  Contracts that are never
// substituted at all show up with a predicate containing anything substitution
// would rewrite, and a plain call suffices; a predicate carried into CodeGen
// still dependent needs one that is value-*dependent* when parsed.  `pre (true)`
// reproduces neither, because a predicate that is already a constant is usable
// without substitution at all.  Two failure modes one axis apart -- and that
// axis is why hand-picked shapes keep catching one and missing the other.
//
// Every shape is measured at both the syntax and the codegen phase, because a
// predicate that reaches CodeGen still dependent is invisible to
// -fsyntax-only.
//
// Mirror: g++.dg/contracts/cpp26/matrix-readers-p3097.C

// expected-no-diagnostics

int mxhelper ();
constexpr int mxhelper_ce () { return 0; }

// --------------------------------------------------------------------------
// A pure virtual in a class template read by its P3097 interface
// wrapper.  There is no definition to instantiate, so the wrapper is the only
// thing that will ever need these contracts -- which makes this the shape that
// exposes both failure modes: contracts never substituted at all, and, for the
// dependent predicate, a predicate that reaches CodeGen still dependent.
//
// Predicate kind: constant -- `true`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual__constant__syntax {

template <class T> struct A {
  virtual int get () const pre (true) = 0;
  int n;
};
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual__constant__syntax

// --------------------------------------------------------------------------
// A pure virtual in a class template read by its P3097 interface
// wrapper.  There is no definition to instantiate, so the wrapper is the only
// thing that will ever need these contracts -- which makes this the shape that
// exposes both failure modes: contracts never substituted at all, and, for the
// dependent predicate, a predicate that reaches CodeGen still dependent.
//
// Predicate kind: call -- `mxhelper () == 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual__call__syntax {

template <class T> struct A {
  virtual int get () const pre (mxhelper () == 0) = 0;
  int n;
};
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual__call__syntax

// --------------------------------------------------------------------------
// A pure virtual in a class template read by its P3097 interface
// wrapper.  There is no definition to instantiate, so the wrapper is the only
// thing that will ever need these contracts -- which makes this the shape that
// exposes both failure modes: contracts never substituted at all, and, for the
// dependent predicate, a predicate that reaches CodeGen still dependent.
//
// Predicate kind: dependent -- `sizeof (T) > 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual__dependent__syntax {

template <class T> struct A {
  virtual int get () const pre (sizeof (T) > 0) = 0;
  int n;
};
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual__dependent__syntax

// --------------------------------------------------------------------------
// A pure virtual in a class template read by its P3097 interface
// wrapper.  There is no definition to instantiate, so the wrapper is the only
// thing that will ever need these contracts -- which makes this the shape that
// exposes both failure modes: contracts never substituted at all, and, for the
// dependent predicate, a predicate that reaches CodeGen still dependent.
//
// Predicate kind: member -- `n >= 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual__member__syntax {

template <class T> struct A {
  virtual int get () const pre (n >= 0) = 0;
  int n;
};
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual__member__syntax

// --------------------------------------------------------------------------
// A virtual with a definition, read by the same wrapper.  The control
// for the pure case: here instantiating the definition would have substituted the
// contracts anyway, so the wrapper must find them already done and not redo the
// work.
//
// Predicate kind: constant -- `true`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace defined_virtual__constant__syntax {

template <class T> struct A {
  virtual int get () const pre (true) { return n; }
  int n;
};
int use (A<int> &a) { return a.get (); }
}  // namespace defined_virtual__constant__syntax

// --------------------------------------------------------------------------
// A virtual with a definition, read by the same wrapper.  The control
// for the pure case: here instantiating the definition would have substituted the
// contracts anyway, so the wrapper must find them already done and not redo the
// work.
//
// Predicate kind: call -- `mxhelper () == 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace defined_virtual__call__syntax {

template <class T> struct A {
  virtual int get () const pre (mxhelper () == 0) { return n; }
  int n;
};
int use (A<int> &a) { return a.get (); }
}  // namespace defined_virtual__call__syntax

// --------------------------------------------------------------------------
// A virtual with a definition, read by the same wrapper.  The control
// for the pure case: here instantiating the definition would have substituted the
// contracts anyway, so the wrapper must find them already done and not redo the
// work.
//
// Predicate kind: dependent -- `sizeof (T) > 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace defined_virtual__dependent__syntax {

template <class T> struct A {
  virtual int get () const pre (sizeof (T) > 0) { return n; }
  int n;
};
int use (A<int> &a) { return a.get (); }
}  // namespace defined_virtual__dependent__syntax

// --------------------------------------------------------------------------
// A virtual with a definition, read by the same wrapper.  The control
// for the pure case: here instantiating the definition would have substituted the
// contracts anyway, so the wrapper must find them already done and not redo the
// work.
//
// Predicate kind: member -- `n >= 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace defined_virtual__member__syntax {

template <class T> struct A {
  virtual int get () const pre (n >= 0) { return n; }
  int n;
};
int use (A<int> &a) { return a.get (); }
}  // namespace defined_virtual__member__syntax

// --------------------------------------------------------------------------
// A virtual declared in-class with the contract and defined
// out-of-line.  The two declarations are different objects on the redeclaration
// chain, which is the shape most likely to defeat an idempotence guard -- a
// "have these already been substituted?" test that compares against only one of
// the two concludes wrongly.
//
// Predicate kind: constant -- `true`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace out_of_line_virtual__constant__syntax {

template <class T> struct A {
  virtual int get () const pre (true);
  int n;
};
template <class T> int A<T>::get () const { return n; }
int use (A<int> &a) { return a.get (); }
}  // namespace out_of_line_virtual__constant__syntax

// --------------------------------------------------------------------------
// A virtual declared in-class with the contract and defined
// out-of-line.  The two declarations are different objects on the redeclaration
// chain, which is the shape most likely to defeat an idempotence guard -- a
// "have these already been substituted?" test that compares against only one of
// the two concludes wrongly.
//
// Predicate kind: call -- `mxhelper () == 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace out_of_line_virtual__call__syntax {

template <class T> struct A {
  virtual int get () const pre (mxhelper () == 0);
  int n;
};
template <class T> int A<T>::get () const { return n; }
int use (A<int> &a) { return a.get (); }
}  // namespace out_of_line_virtual__call__syntax

// --------------------------------------------------------------------------
// A virtual declared in-class with the contract and defined
// out-of-line.  The two declarations are different objects on the redeclaration
// chain, which is the shape most likely to defeat an idempotence guard -- a
// "have these already been substituted?" test that compares against only one of
// the two concludes wrongly.
//
// Predicate kind: dependent -- `sizeof (T) > 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace out_of_line_virtual__dependent__syntax {

template <class T> struct A {
  virtual int get () const pre (sizeof (T) > 0);
  int n;
};
template <class T> int A<T>::get () const { return n; }
int use (A<int> &a) { return a.get (); }
}  // namespace out_of_line_virtual__dependent__syntax

// --------------------------------------------------------------------------
// A virtual declared in-class with the contract and defined
// out-of-line.  The two declarations are different objects on the redeclaration
// chain, which is the shape most likely to defeat an idempotence guard -- a
// "have these already been substituted?" test that compares against only one of
// the two concludes wrongly.
//
// Predicate kind: member -- `n >= 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace out_of_line_virtual__member__syntax {

template <class T> struct A {
  virtual int get () const pre (n >= 0);
  int n;
};
template <class T> int A<T>::get () const { return n; }
int use (A<int> &a) { return a.get (); }
}  // namespace out_of_line_virtual__member__syntax

// --------------------------------------------------------------------------
// A pure virtual reached after an explicit instantiation of the class
// template, which instantiates the members' declarations on a different schedule
// from an implicit use.
//
// Predicate kind: constant -- `true`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual_explicit_inst__constant__syntax {

template <class T> struct A {
  virtual int get () const pre (true) = 0;
  int n;
};
template struct A<int>;
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual_explicit_inst__constant__syntax

// --------------------------------------------------------------------------
// A pure virtual reached after an explicit instantiation of the class
// template, which instantiates the members' declarations on a different schedule
// from an implicit use.
//
// Predicate kind: call -- `mxhelper () == 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual_explicit_inst__call__syntax {

template <class T> struct A {
  virtual int get () const pre (mxhelper () == 0) = 0;
  int n;
};
template struct A<int>;
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual_explicit_inst__call__syntax

// --------------------------------------------------------------------------
// A pure virtual reached after an explicit instantiation of the class
// template, which instantiates the members' declarations on a different schedule
// from an implicit use.
//
// Predicate kind: dependent -- `sizeof (T) > 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual_explicit_inst__dependent__syntax {

template <class T> struct A {
  virtual int get () const pre (sizeof (T) > 0) = 0;
  int n;
};
template struct A<int>;
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual_explicit_inst__dependent__syntax

// --------------------------------------------------------------------------
// A pure virtual reached after an explicit instantiation of the class
// template, which instantiates the members' declarations on a different schedule
// from an implicit use.
//
// Predicate kind: member -- `n >= 0`.  Phase: syntax.
// --------------------------------------------------------------------------
namespace pure_virtual_explicit_inst__member__syntax {

template <class T> struct A {
  virtual int get () const pre (n >= 0) = 0;
  int n;
};
template struct A<int>;
struct D : A<int> { int get () const override { return n; } };
int use (A<int> &a) { return a.get (); }
}  // namespace pure_virtual_explicit_inst__member__syntax
