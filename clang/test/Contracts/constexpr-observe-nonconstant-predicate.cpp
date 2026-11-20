// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify %s

// A predicate that is not a core constant expression in a manifestly
// constant-evaluated context is a contract violation ([basic.contract.eval]);
// under observe that is a diagnostic and evaluation continues, so each
// initializer below stays constant (DECISIONS.md M1).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-observe-nonconstant-predicate.C

int g(int x) { return x; } // not constexpr
constexpr int f(int x) pre(g(x) > 0) { return x; } // expected-warning 2 {{contract failed during execution of constexpr function}}
constexpr int y = f(1);
static_assert(y == 1);
constexpr int h(int x) { contract_assert(g(x) > 0); return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int z = h(1);
int g2 = f(1); // constant-initialization recheck
