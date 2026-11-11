// RUN: %clang -std=c++23 -fno-contracts-p3400 -fcontracts-p3850 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty --check-prefix=QUIET %s

// DECISIONS.md E175: -fcontracts-p3850, -fcontracts-p3400 and
// -fcontracts-p4451 are additive, whatever their order.  Columns of the name:
// p3850, p3400, p4451; y given, n given as -fno-, x not given; -rev: the -fno- options first.
// Expected: P3400 off, P4451 on, contracts on.
// -std=c++23, so that only the options can turn contracts on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-ynx-rev.C

// QUIET-NOT: warning:

// expected-no-diagnostics

#ifndef __cpp_contracts
#error "contracts are on"
#endif
#ifdef __cpp_contracts_labels
#error "P3400 is off"
#endif
#ifndef __clang_contracts_p4451
#error "P4451 is on"
#endif
#if !(__cpp_contracts > 202502L)
#error "P4451 raises __cpp_contracts"
#endif

int f(int x) pre(x > 0) { return x; }
