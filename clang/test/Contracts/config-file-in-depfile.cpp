// RUN: %clang -std=c++26 -fcontracts -fcontract-configuration-file=%S/config-file-in-depfile.json -MD -MF %t.d -c %s -o %t.o
// RUN: FileCheck --input-file=%t.d %s

// The contract configuration file changes the generated code, so it is
// recorded in the -MD dependency file (as -fsanitize-ignorelist= is).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/config-file-in-depfile.C

// CHECK: config-file-in-depfile.json

int f(int x) pre(x > 0) { return x; }
