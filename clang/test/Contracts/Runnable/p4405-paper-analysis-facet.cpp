// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t -Xclang -verify
// RUN: not --crash %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "P3400R5 Analysis Evaluation Configuration": the diagnosed label
// (an analysis_semantic facet) asks for the assertion to be diagnosed,
// leaving its runtime evaluations to the build: built to enforce, it warns
// that the assertion cannot be proven, and f(-1) is still a violation at
// run time.  The paper's example, with its comment
// (p4405-paper-analysis-facet-ignore.cpp and -prove.cpp vary the build).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-facet.C

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
  f(-1);  // Violation at runtime
  std::puts("done");
}

// CHECK: violation: x > 0
