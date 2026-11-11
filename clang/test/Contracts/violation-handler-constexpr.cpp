// REQUIRES: contracts-libcxx
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify -DSPEC=consteval %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify -DSPEC=constexpr %s

// A consteval or constexpr ::handle_contract_violation is implicitly inline
// ([dcl.constexpr]), so it cannot replace the violation handler
// ([dcl.fct.def.replace]) and is diagnosed at its declaration, like the
// explicit inline in violation-handler-signature-wording.cpp.  The observe
// semantic is pinned because it is one of the two that call the handler.
// The same function in a namespace is not the handler and is fine.
//
// GCC mirror: gcc/testsuite/g++.dg/contracts/cpp26/pr128202.C (consteval)
// and violation-handler-constexpr.C (constexpr).

#include <contracts>

using std::contracts::contract_violation;

SPEC void handle_contract_violation(const contract_violation &) {} // expected-error {{'::handle_contract_violation' must not be declared 'inline'}}

namespace N {
SPEC void handle_contract_violation(const contract_violation &) {} // OK, not the replacement handler
}

int main() { contract_assert(false); }
