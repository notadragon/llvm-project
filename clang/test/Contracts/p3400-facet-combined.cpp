// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=enforce -fsyntax-only

// compute_semantic on combined labels.

#include <contracts>

// review | empty_label — compute_semantic still works
void f(int x) pre<review | empty_label>(x > 0) {}

// empty_label | review — compute_semantic still works
void g(int x) pre<empty_label | review>(x > 0) {}

// review | review — chaining: left then right
void h(int x) pre<review | review>(x > 0) {}
