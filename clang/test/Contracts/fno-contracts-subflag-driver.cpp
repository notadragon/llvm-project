// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++23 -fcontracts-p3400 -fno-contracts-p3400 %s 2>&1 | FileCheck %s
// RUN: %clang --target=x86_64-unknown-linux-gnu -std=c++23 -fcontracts-p3400 -fno-contracts-p3400 -fsyntax-only -Xclang -verify %s

// A per-paper sub-flag that a later -fno-contracts-pNNNN cancels does not
// count: below C++26 the driver forwards no -fcontracts and links no
// -lcontracts, and a contract is a syntax error.  The C shape is
// fno-contracts-p4299-driver.c.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/fno-contracts-subflag.C

// CHECK-NOT: "-fcontracts"
// CHECK-NOT: "-lcontracts"

int f(int x) pre(x > 0) { return x; } // expected-error {{expected function body after function declarator}}
