// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s

// A compute_semantic facet that is not usable in a constant expression is
// reported at the contract.  (The GCC test also covers a function-template
// precondition; Clang diagnoses that one eagerly at the declaration as well
// as on instantiation, so it is left out here.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/facet-constant-location.C

#include <contracts>

using std::contracts::evaluation_semantic;

struct L {
  using assertion_control_object = L;
  evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
constexpr L l{};

void g() {
  // expected-error@+1 {{must be usable in a constant expression}}
  contract_assert<l>(true);
}

// REQUIRES: contracts-libcxx
