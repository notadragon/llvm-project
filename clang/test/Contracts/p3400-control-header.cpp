// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// After #include <contracts>, empty_label is available unqualified
// in assertion-control expressions via the header's
// using contract_control namespace directive.

#include <contracts>

void f(int x) pre<empty_label>(x > 0) {}

int g(int x) post<empty_label>(r: r >= 0) { return x; }

void h() {
  contract_assert<empty_label>(true);
}
