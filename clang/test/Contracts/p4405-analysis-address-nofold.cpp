// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fno-contracts-prove-fold %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function"): only folding can prove a
// precondition for a call through the address of its function, so with
// -fno-contracts-prove-fold even a tautological precondition that allows no
// runtime semantic is diagnosed where the address is taken
// (p4405-analysis-address-tautological.cpp has it with folding).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address-nofold.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

void t1(int x) pre<always_prove>(true); // expected-note {{precondition here}}
void t2(int x) pre<always_prove>(sizeof(x) == sizeof(int)); // expected-note {{precondition here}}

struct DT {
  ~DT() pre<always_prove>(true); // expected-note {{precondition here}}
};

DT dt1; // expected-error {{contract assertion cannot be proven for a call through the address of '~DT'}} expected-note {{'dt1', of static storage duration, is destroyed by a call through the address of its destructor}}

void (*p1)(int) = t1; // expected-error {{contract assertion cannot be proven for a call through the address of 't1'}}
void (*p2)(int) = &t2; // expected-error {{contract assertion cannot be proven for a call through the address of 't2'}}
