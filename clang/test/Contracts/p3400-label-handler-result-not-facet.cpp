// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s

// A handle_contract_violation whose result is neither void nor convertible to
// violation_handled does not provide the local_violation_label facet (the
// concept requires one or the other), so the label simply has none and the
// program is well formed.  It used to be taken as the facet and rejected
// (CLANG-618).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-handler-result-not-facet.C
// expected-no-diagnostics

#include <contracts>

struct lab_t {
  using assertion_control_object = lab_t;
  int handle_contract_violation(const std::contracts::contract_violation &) const {
    return 1;
  }
};
constexpr lab_t lab{};

int f(int x) pre<lab>(x > 0) { return x; }

// A void result, or one converting to violation_handled, is the facet.
struct void_lab_t {
  using assertion_control_object = void_lab_t;
  void handle_contract_violation(const std::contracts::contract_violation &) const {}
};
constexpr void_lab_t void_lab{};
int g(int x) pre<void_lab>(x > 0) { return x; }

// REQUIRES: contracts-libcxx
