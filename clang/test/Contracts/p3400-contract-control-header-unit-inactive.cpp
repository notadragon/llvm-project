// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %clangxx -std=c++26 -fcontracts-p3400 %libcxx_flags -w -fmodule-header=system \
// RUN:   -xc++-system-header contracts -o %t/contracts.pcm
// RUN: %clangxx -std=c++26 -fcontracts-p3400 %libcxx_flags -Wno-experimental-header-units \
// RUN:   -fmodule-file=%t/contracts.pcm -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400: <contracts> as a header unit carries its contract_control
// using-directive (D3400R6 [module.interface]); the directive from
// "import <contracts>;" is active inside an assertion-control expression and
// inactive everywhere else, including a contract predicate.  (HU survey case
// 8(b).)
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-control-3_a.X and _b.C

import <contracts>;

// Active: an assertion-control expression.
int f(int x) pre<review>(x > 0) pre<terminating>(x > 1) { return x; }

// Inactive: ordinary lookup, at namespace scope, in a function body and in
// a predicate.
auto leak1 = review; // expected-error {{use of undeclared identifier 'review'}}
auto leak2 = terminating; // expected-error {{use of undeclared identifier 'terminating'}}
// expected-note@*:* {{'std::contracts::labels::terminating' declared here}}

void g() {
  auto leak3 = review; // expected-error {{use of undeclared identifier 'review'}}
}

int h(int x) pre(&review != nullptr); // expected-error {{use of undeclared identifier 'review'}}
