// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %s %libcxx_flags -o %t && %t | FileCheck %s

// Contracts on friend declarations and static members are in a complete-class
// context: they may name a private member declared later.  Mirror of GCC's
// g++.dg/contracts/cpp26/contracts-friend1.C.

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("violation %u: %s\n", (unsigned)v.location().line(), v.comment());
}
struct X {
  friend void fn0(X x) pre (x.a > 0) { }

  static void fns0(X x) pre ( x.a > 0 ) { }
  static void fns1(X x) pre ( x.a > 0 );
  static void fns2(X x) pre ( x.a > 0 );

  friend void fn(X &x) { x.a = -5; }

private:
  int a{10};
};
void X::fns1(X x) { }
void X::fns2(X x) pre ( x.a > 0 ) { }

int main(int, char**) {
  X x;
  fn(x); // no contract

  fn0(x);

  X::fns0(x);
  X::fns1(x);
  X::fns2(x);
  return 0;
}


// CHECK: violation 14: x.a > 0
// CHECK-NEXT: violation 16: x.a > 0
// CHECK-NEXT: violation 17: x.a > 0
// CHECK-NEXT: violation 26: x.a > 0
