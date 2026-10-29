// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "P3400R5 Analysis Evaluation Configuration": the diagnosed label
// only strengthens the analysis evaluation, so a configured prove is kept
// and the assertion that cannot be proven is an error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-facet-prove.C

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
  contract_assert<diagnosed>(x > 0);  // expected-error {{contract assertion cannot be proven}}
}
