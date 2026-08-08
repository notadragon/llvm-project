// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only \
// RUN:   2>&1 | FileCheck %s

// E6: a P3100 implicit UB check does not participate in constant evaluation.
// The constexpr evaluator still rejects the undefined operation directly; the
// configured contract semantic (observe) does not turn the UB into a
// recoverable contract violation.  (P3100 instrumentation is a runtime facility;
// constant expressions already forbid the UB it guards.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-constexpr.C, adapted to
// Clang's constexpr diagnostic wording.)

constexpr int kMin = -__INT_MAX__ - 1;

constexpr int divz(int a, int b) { return a / b; } // division by zero
constexpr int ovf(int a, int b) { return a / b; }  // INT_MIN / -1 overflow

// Both are rejected as non-constant regardless of the observe semantic.
// CHECK-COUNT-2: must be initialized by a constant expression
constexpr int z = divz(1, 0);
constexpr int o = ovf(kMin, -1);
