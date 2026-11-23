// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t -Xclang -verify
// RUN: %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405 ("Controlling the Analysis Semantic with Labels"): the standard
// diagnose label sets only the analysis semantic, so built to observe, a
// precondition, a postcondition and a contract_assert with it each warn
// at compile time that they cannot be proven, and their runtime
// evaluations are still observe: each violation is reported and the
// program continues.  (p4405-std-labels-diagnose-enforce-run.cpp: built to
// enforce.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-std-labels-diagnose-observe-run.C

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int violations;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  std::printf("violation: %s, %s\n", v.comment(),
              v.semantic() == std::contracts::evaluation_semantic::observe
                  ? "observe"
                  : "other");
}

[[gnu::noinline]] void pre_d(int x) pre<diagnose>(x > 0) {} // expected-note {{the predicate of the contract assertion}}
[[gnu::noinline]] int post_d(int x) post<diagnose>(r : r > 0) { return x; } // expected-warning {{contract assertion cannot be proven}}
[[gnu::noinline]] void assert_d(int x) {
  contract_assert<diagnose>(x > 0); // expected-warning {{contract assertion cannot be proven}}
}

int main(int argc, char **) {
  int n = argc - 2;
  pre_d(n); // expected-warning {{contract assertion cannot be proven}}
  post_d(n);
  assert_d(n);
  std::printf("after: %d\n", violations);
  if (violations != 3)
    std::abort();
}

// CHECK: violation: x > 0, observe
// CHECK-NEXT: violation: r : r > 0, observe
// CHECK-NEXT: violation: x > 0, observe
// CHECK-NEXT: after: 3
