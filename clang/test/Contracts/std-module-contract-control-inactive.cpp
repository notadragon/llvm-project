// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: %clangxx -std=c++26 -fcontracts-p3400 %libcxx_flags -Wno-reserved-module-identifier \
// RUN:   --precompile %libcxx_moduledir/std.cppm -o %t/std.pcm
// RUN: %clangxx -std=c++26 -fcontracts-p3400 %libcxx_flags -fmodule-file=std=%t/std.pcm \
// RUN:   -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx, libcxx-std-module

// P3400: the contract_control directive "import std;" exports
// (std-module-contracts-labels.cpp) is active inside an assertion-control
// expression and inactive everywhere else, including a contract predicate.
// (HU survey case 8(c).)
//
// Mirror: gcc/testsuite/g++.dg/modules/std-contract-control-inactive.C

import std;

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
