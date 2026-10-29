// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -Wno-contract-unproven %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405: -Wno-contract-unproven silences the warning for taking the
// address of a function whose precondition allows only diagnose
// (always_diagnose), but not the error for one that allows only prove
// (always_prove; p4405-analysis-address.cpp).  A precondition that only the
// configuration analyzes is not diagnosed at all
// (p4405-analysis-address-flag.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address-wno.C

#include <contracts>

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr std::contracts::evaluation_semantic_set
    allowed_semantics{std::contracts::evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr std::contracts::evaluation_semantic_set
    allowed_semantics{std::contracts::evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int f(int x) pre(x > 0);
int fd(int x) pre<always_diagnose>(x > 0);
int fp(int x) pre<always_prove>(x > 0); // expected-note {{precondition here}}

int (*p)(int) = f;
int (*d)(int) = fd;
int (*q)(int) = fp; // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
