// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -verify %s

// P3097 x constant evaluation: a constexpr virtual function with a contract,
// called through a base reference in a constant expression.  Virtual calls are
// permitted in constant expressions since C++20, so this should be usable at
// compile time (the postcondition holds here).  (GCC mirror:
// p3097-virtual-constexpr.C; GCC currently rejects this as BUG-9.)

// expected-no-diagnostics
struct Base {
  constexpr virtual int f() const post(r: r > 0) { return 1; }
  constexpr virtual ~Base() = default;
};

struct Derived : Base {
  constexpr int f() const override { return 5; }
};

constexpr int test() { Derived d; const Base& b = d; return b.f(); }
constexpr int y = test();
