// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405: without -fcontracts-p4405, <contracts> declares none of the
// standard labels prove, diagnose, always_prove and always_diagnose, while
// the P3400 labels are there (p4405-std-labels.cpp: with the flag).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-std-labels-without-flag.C

#include <contracts>

namespace L = std::contracts::labels;

#ifdef __cpp_contracts_prove
#error "__cpp_contracts_prove defined without -fcontracts-p4405"
#endif

constexpr auto ok = L::always_enforce;

constexpr auto a = L::prove;           // expected-error {{no member named 'prove' in namespace 'std::contracts::labels'}}
constexpr auto b = L::diagnose;        // expected-error {{no member named 'diagnose' in namespace 'std::contracts::labels'}}
constexpr auto c = L::always_prove;    // expected-error {{no member named 'always_prove' in namespace 'std::contracts::labels'; did you mean 'always_ignore'?}}
constexpr auto d = L::always_diagnose; // expected-error {{no member named 'always_diagnose' in namespace 'std::contracts::labels'; did you mean 'always_ignore'?}}
// expected-note@*:* 2 {{'always_ignore' declared here}}
L::prove_t e;                          // expected-error {{no type named 'prove_t' in namespace 'std::contracts::labels'}}
L::always_diagnose_t f;                // expected-error {{no type named 'always_diagnose_t' in namespace 'std::contracts::labels'}}

void g(int x) pre<prove>(x > 0);       // expected-error {{use of undeclared identifier 'prove'}}
