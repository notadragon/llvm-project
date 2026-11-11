// RUN: %clangxx -std=c++23 -fno-contracts -### %s 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++23 -fcontracts -fno-contracts -### %s 2>&1 | FileCheck %s

// The driver does not link the contracts runtime for -fno-contracts.
// (Sub-flag negation is fno-contracts-subflag-driver.cpp.)

// CHECK-NOT: "-fcontracts"
// CHECK-NOT: -lcontracts
