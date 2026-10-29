// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s
// XFAIL: *

// OPEN BUG (CLANG-628 in this fork's bug-reports/; stock Clang has it,
// llvm/llvm-project#29713; no contracts involved): top-level cv-qualifiers
// of a parameter are not part of the function type ([dcl.fct]/5), so in a
// definition the parameter variable takes them from the definition alone.
// For an out-of-class definition of a member of a class template whose
// parameter type is dependent, Clang takes them from the in-class
// declaration instead: it rejects assigning to `x` in A<T>::m1 and accepts
// it in A<T>::m2, the reverse of what is right.  The same holds for a member
// template of a class template.  Non-dependent parameter types are right.
// Clang also prints the type of a `const T` parameter with T = const int
// as "const const int".
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-param-cv-member-definition.C,
// which passes.

template <class T> struct A {
  void m1(const T x);
  void m2(T x);
  template <class U> void m4(const T x, U);
};
template <class T> void A<T>::m1(T x) { x = 1; }         // OK
template <class T> void A<T>::m2(const T x) { // expected-note {{variable 'x' declared const here}}
  x = 1;  // expected-error {{cannot assign to variable 'x' with const-qualified type 'const int'}}
}
template <class T> template <class U>
void A<T>::m4(T x, U) { x = 1; }                       // OK
template struct A<int>;  // expected-note {{in instantiation of member function 'A<int>::m2' requested here}}
template void A<int>::m4(int, char);

template <class T> struct C {
  template <class U> void m(const U x);
};
template <class T> template <class U>
void C<T>::m(U x) { x = 1; }                           // OK
template void C<int>::m(int);

struct S {
  void n(const int x);
};
void S::n(int x) { x = 1; }                            // OK, not dependent
