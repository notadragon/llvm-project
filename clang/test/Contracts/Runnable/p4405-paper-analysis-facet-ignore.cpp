// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t -Xclang -verify
// RUN: %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "P3400R5 Analysis Evaluation Configuration": the diagnosed label
// of p4405-paper-analysis-facet.cpp built to ignore contract assertions: the
// analysis evaluation still warns, and the runtime evaluation is ignore,
// so f(-1) reports nothing.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-facet-ignore.C

#include <contracts>  // `evaluation_semantic`, `contract_violation`
#include <cstdio>  // `std::printf`, `std::fflush`, `std::puts`

// The paper's diagnosed label.
struct diagnosed_t {
  using assertion_control_object = diagnosed_t;

  constexpr std::contracts::evaluation_semantic analysis_semantic() const {
    return std::contracts::evaluation_semantic::diagnose;
  }
};
constexpr diagnosed_t diagnosed{};

// A violation handler that reports each violation; with *enforce* the
// program then terminates.
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  std::printf("violation: %s\n", v.comment());
  std::fflush(stdout);
}

void f(int x)
{
  contract_assert<diagnosed>(x > 0);  // OK, warns: cannot be proven expected-warning {{contract assertion cannot be proven}}
}

int main()
{
  f(-1);
  std::puts("done");
}

// CHECK: done
