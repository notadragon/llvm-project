// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-implicit-constexpr.json \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

// a P3100 implicit UB check does not participate in constant evaluation.
// The constexpr evaluator still rejects the undefined operation directly; the
// configured contract semantic (observe) does not turn the UB into a
// recoverable contract violation.  (P3100 instrumentation is a runtime facility;
// constant expressions already forbid the UB it guards.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-constexpr.C, adapted to
// Clang's constexpr diagnostic wording.)  The configuration file is what turns
// the implicit checks on: -fcontract-evaluation-semantic= does not reach
// implicit assertions.

constexpr int kMin = -__INT_MAX__ - 1;

constexpr int divz(int a, int b) { return a / b; } // division by zero
constexpr int ovf(int a, int b) { return a / b; }  // INT_MIN / -1 overflow

// Both are rejected as non-constant regardless of the observe semantic, with
// the evaluator's own reason and nothing from the contract machinery.
// CHECK-NOT: contract
// CHECK: must be initialized by a constant expression
// CHECK: note: division by zero
// CHECK-NOT: contract
// CHECK: must be initialized by a constant expression
// CHECK: note: value 2147483648 is outside the range of representable values
// CHECK-NOT: contract
constexpr int z = divz(1, 0);
constexpr int o = ovf(kMin, -1);

// REQUIRES: contracts-libcxx
