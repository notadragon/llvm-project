// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fsyntax-only

// empty | empty has no compute_semantic (constraint not satisfied),
// so semantic is left unchanged.

#include <contracts>

void f(int x) pre<empty_label | empty_label>(x > 0) {}

// Triple chain of empties — still no compute_semantic
void g(int x) pre<empty_label | empty_label | empty_label>(x > 0) {}
