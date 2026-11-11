// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3097 bumps __cpp_contracts past P2900's 202502L, to the date of the
// implemented revision until it is adopted.
#if __cpp_contracts <= 202502L
#error "__cpp_contracts not bumped with -fcontracts-p3097"
#endif

struct Base {
  virtual void f() pre(true);
};

struct Derived : Base {
  void f() override pre(true);
};
