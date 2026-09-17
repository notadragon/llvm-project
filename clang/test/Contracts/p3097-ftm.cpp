// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3097R3 bumps __cpp_contracts to 202606L.
#if __cpp_contracts < 202606L
#error "__cpp_contracts not bumped to 202606L with -fcontracts-p3097"
#endif

struct Base {
  virtual void f() pre(true);
};

struct Derived : Base {
  void f() override pre(true);
};
