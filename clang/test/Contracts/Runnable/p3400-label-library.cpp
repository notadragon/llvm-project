// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// Library empty_label works with label syntax.

#include <contracts>

void f(int x) pre<std::contracts::labels::empty_label>(x > 0) {}

int g(const int x) post<std::contracts::labels::empty_label>(r: r >= 0) { return x; }

void h() {
  contract_assert<std::contracts::labels::empty_label_t{}>(true);
}
