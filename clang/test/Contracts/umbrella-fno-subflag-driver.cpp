// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 -fcontracts-p3850 -fno-contracts-p3400 -c %s 2>&1 | FileCheck %s
// RUN: %clang --target=x86_64-unknown-linux-gnu -std=c++26 -fcontracts-p3850 -fno-contracts-p3400 -fsyntax-only -Xclang -verify %s

// The driver forwards the last of each per-paper sub-flag pair, so
// -fcontracts-p3850 -fno-contracts-p3400 reaches cc1 with the -fno- and P3400
// is off.  The cc1 half is umbrella-fno-subflag.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/umbrella-fno-subflag.C

// CHECK: "-fno-contracts-p3400"

struct L { using assertion_control_object = L; };
constexpr L lbl{};
int f(int x) pre<lbl>(x > 0) { return x; } // expected-error {{assertion-control labels require '-fcontracts-p3400'}}
