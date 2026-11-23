// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-group-evaluation-semantic=observe -fcontract-evaluation-semantic=enforce -fsyntax-only %s 2>&1 | FileCheck %s

// -fcontract-group-evaluation-semantic= takes group:semantic; a bare semantic
// is rejected, as GCC rejects it (the default semantic has its own option,
// -fcontract-evaluation-semantic=).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/bare-group-semantic.C

// CHECK: error: invalid '-fcontract-group-evaluation-semantic=' argument 'observe'; expected 'group:semantic'

void f() { contract_assert(false); }
