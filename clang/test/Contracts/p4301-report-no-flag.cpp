// D4301: contract_violation::report() must NOT be available without
// -fcontracts-p4301, even though contracts (and the rest of the
// contract_violation API) are otherwise enabled.
// (GCC mirror: g++.dg/contracts/cpp26/p4301-report-no-flag.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fsyntax-only -Xclang -verify %libcxx_flags

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  v.report(); // expected-error {{no member named 'report' in 'std::contracts::contract_violation'}}
}
