// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3097 bumps __cpp_contracts past P2900's 202502L and P4451R2's 202611L,
// to its fixed 202612L (notadragon_wg21 DECISIONS.md E222 (3)).
#if __cpp_contracts != 202612L
#error "__cpp_contracts not 202612L with -fcontracts-p3097"
#endif

struct Base {
  virtual void f() pre(true);
};

struct Derived : Base {
  void f() override pre(true);
};
