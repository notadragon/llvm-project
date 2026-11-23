// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -Wno-contracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// The -Wcontracts umbrella includes -Wcontract-invalid-label-facet, so
// -Wno-contracts silences it.
//
// No GCC mirror: GCC has no -Wcontracts umbrella (-Wcontracts is an
// unrecognized option there).

#include <contracts>

struct nonconst_t {
  using assertion_control_object = nonconst_t;
  void handle_contract_violation(const std::contracts::contract_violation &) {}
};
constexpr nonconst_t nonconst{};

void f(int x) pre<nonconst>(x > 0) {}

// REQUIRES: contracts-libcxx
