// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t | FileCheck %s

// empty | empty has no compute_semantic (constraint not satisfied),
// so semantic is left unchanged.
//
// The handler prints the semantic each call was evaluated with (observe is
// 2, enforce 3), which is what the semantic transformation changes.

#include <contracts>

#include <cstdio>

// Every call below violates its precondition.  The handler prints what it
// was given and throws, so each call reports and returns, whatever its
// semantic.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("semantic %d\n", static_cast<int>(v.semantic()));
  throw 0;
}

template <class F> void call(const char *name, F f) {
  std::printf("%s: ", name);
  try {
    f();
    std::printf("no violation\n");
  } catch (int) {
  }
}

void f(int x) pre<empty_label | empty_label>(x > 0) {}

// Triple chain of empties -- still no compute_semantic
void g(int x) pre<empty_label | empty_label | empty_label>(x > 0) {}

int main() {
  call("f", [] { f(-1); });
  call("g", [] { g(-1); });
}

// CHECK: f: semantic 3
// CHECK-NEXT: g: semantic 3

// REQUIRES: contracts-libcxx, native
