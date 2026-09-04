// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s -fcontracts
// XFAIL: *

// [dcl.contract.func]/7 is never applied to a DEPENDENT parameter type.
//
// The rule is about a function: a specialization whose non-reference
// parameter deduces to a non-const type and is odr-used by a postcondition
// predicate is ill-formed, even though the template itself is fine (a valid
// specialization exists, so [temp.res.general] does not make it ill-formed
// NDR).  Clang checks the rule when the parameter type is concrete -- see
// deducing-this-postcondition-param.cpp, which passes -- and never rechecks
// it at instantiation, so every shape below is accepted silently.  GCC
// diagnoses all four.
//
// This is not specific to deducing this: DeducedOrdinary and InClassTemplate
// below have no explicit object parameter at all.  The axis is "the parameter
// type was dependent when the contract was first seen".
//
// GCC mirror: g++.dg/contracts/cpp26/deducing-this-postcondition-param.C

struct S {
  int x = 0;
};

// A deduced explicit object parameter, by value.  Deduction from a by-value
// parameter drops the argument's top-level cv-qualification, so Self deduces
// to the unqualified class type and the parameter is non-const.
struct DeducedByValue : S {
  // expected-error@+1 {{must be declared const}}
  template <class Self> void f(this Self self) post(self.x == 0);
};

void use_deduced_by_value() {
  DeducedByValue a;
  a.f();
}

// The same, called on a const object: deduction still drops the const, so
// this is ill-formed for the same reason.
void use_deduced_by_value_on_const() {
  const DeducedByValue a;
  a.f();
}

// CONTROL, no explicit object parameter: an ordinary function template whose
// by-value parameter type is deduced non-const.
// expected-error@+1 {{must be declared const}}
template <class T> void deduced_ordinary(T t) post(t.x == 0);

void use_deduced_ordinary() {
  S s;
  deduced_ordinary(s);
}

// CONTROL, dependent but not deduced: a class template's member with a
// by-value parameter of the template parameter type.
template <class T> struct InClassTemplate {
  // expected-error@+1 {{must be declared const}}
  void f(T t) post(t.x == 0);
};

void use_in_class_template() {
  InClassTemplate<S> x;
  S s;
  x.f(s);
}

// An explicit object parameter of the enclosing class template's own type.
template <class T> struct ExplicitObjectInClassTemplate : S {
  // expected-error@+1 {{must be declared const}}
  void f(this ExplicitObjectInClassTemplate self) post(self.x == 0);
};

void use_explicit_object_in_class_template() {
  ExplicitObjectInClassTemplate<int> x;
  x.f();
}
