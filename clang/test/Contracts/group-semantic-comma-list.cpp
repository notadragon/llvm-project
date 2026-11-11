// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-group-evaluation-semantic=a:observe,b:ignore %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

// -fcontract-group-evaluation-semantic= accepts a comma-separated list of
// group:semantic elements, applying each.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-semantic-comma-list.C

#include <cstdio>
#include <contracts>
using namespace std::contracts::labels;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::fprintf(stderr, "violation sem=%d\n", (int)v.semantic());
}

int f(int x) pre<"a.x"group>(x > 0) { return x; }
int g(int x) pre<"b.y"group>(x > 0) { return x; }

int main() {
  f(-1);
  g(-1);
  std::fputs("returned\n", stderr);
}

// CHECK: violation sem=2
// CHECK-NOT: violation
// CHECK: returned

// REQUIRES: contracts-libcxx, native
