// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// Controls for the CLANG-8 open-bug files
// OpenBugs/this-in-xobj-trailing-return-type.cpp and
// OpenBugs/this-in-xobj-noexcept-specifier.cpp, kept out of them because lit
// XFAILs a whole file and would mask a regression here.
//
// [expr.prim.this]/3: `this` shall not appear within the declaration of a
// static member function (rejected today, in both positions) and may appear
// in that of an implicit object member function (accepted).

struct S {
  int x;

  // expected-error@+1 {{'this' cannot be used in a static member function declaration}}
  static auto g1() -> decltype(this->x);
  auto h1() -> decltype(this->x);

  // expected-error@+1 {{'this' cannot be used in a static member function declaration}}
  static void g2() noexcept(noexcept(this->x));
  void h2() noexcept(noexcept(this->x));
};
