// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t -Xclang -verify
// RUN: not --crash %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405 ("Controlling the Analysis Semantic with Labels"), E198 reading 1:
// the standard diagnose label sets only the analysis semantic, so built
// to enforce, pre<diagnose> warns at compile time that it cannot be proven
// and its runtime evaluation is still enforce: f(-1) is a violation, and
// the program terminates after the handler returns.
// (p4405-std-labels-diagnose-observe-run.cpp: built to observe.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-std-labels-diagnose-enforce-run.C

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("violation: %s, %s\n", v.comment(),
              v.semantic() == std::contracts::evaluation_semantic::enforce
                  ? "enforce"
                  : "other");
  std::fflush(stdout);
}

[[gnu::noinline]] void f(int x) pre<diagnose>(x > 0) {} // expected-note {{the predicate of the contract assertion}}

int main(int argc, char **) {
  f(argc - 2); // expected-warning {{contract assertion cannot be proven}}
  std::puts("after");
}

// CHECK: violation: x > 0, enforce
// CHECK-NOT: after
