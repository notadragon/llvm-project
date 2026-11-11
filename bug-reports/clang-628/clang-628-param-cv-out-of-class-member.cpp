// clang-628-param-cv-out-of-class-member.cpp                     -*-C++-*-
//
// CLANG-628: an out-of-class definition of a member of a class template
// takes its parameters' top-level cv-qualifiers from the in-class
// declaration instead of its own, for parameters of dependent type.  No
// contracts involved, plain C++.
//
// [dcl.fct]/5: top-level cv-qualifiers of a parameter are not part of the
// function type; in a definition, the parameter variable's type is the one
// the definition declares.
//
// Expected: an error on line B only (assignment to a const parameter).
// Actual:   an error on line A ("cannot assign to variable 'x' with
//           const-qualified type 'const int'") and none on line B.
//
// clang++ -std=c++20 -fsyntax-only clang-628-param-cv-out-of-class-member.cpp

template <class T> struct A {
  void m(const T x);
  void n(T x);
};
template <class T> void A<T>::m(T x) { x = 1; }        // A: rejected (bug)
template <class T> void A<T>::n(const T x) { x = 1; }  // B: accepted (bug)
template struct A<int>;
