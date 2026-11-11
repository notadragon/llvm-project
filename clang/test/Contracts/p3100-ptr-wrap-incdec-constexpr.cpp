// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce
// expected-no-diagnostics

// P3100: with the pointer-wrap check configured, pointer increments and
// decrements are still constant expressions: the check is code generation
// only.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-incdec-constexpr.C)

constexpr int f() {
  int a[4]{1, 2, 3, 4};
  int *p = a;
  ++p;
  p++;
  --p;
  int *q = p--;
  return *q * 10 + *p + (p - a);
}
static_assert(f() == 21);
