// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -o %t -Xclang -verify
// RUN: %t | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "Local Proofs": contract_assert with always_diagnose warns when
// it cannot be proven, and with always_prove must be proven (here by the
// proof shadow, optimized).  Neither is ever checked at run time: f(-1)
// evaluates neither predicate.  The paper's example, with its comments.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-local-assert.C

#include <cstdio>
#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// The paper's always_prove label, and always_diagnose, "defined the same
// way".
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void f(int x)
{
  contract_assert<always_diagnose>(x > 0);  // OK, warns: cannot be proven expected-warning {{contract assertion cannot be proven}}
  if (x > 0) {
    contract_assert<always_prove>(x > 0);  // OK, proven
  }
}

int main()
{
  f(-1);  // Neither predicate is evaluated, so nothing is reported.
  std::puts("done");
}

// CHECK: done
