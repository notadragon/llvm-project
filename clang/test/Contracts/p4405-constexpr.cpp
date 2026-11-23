// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-constexpr.json -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-constexpr.json -emit-llvm-only -fexperimental-new-constant-interpreter -verify %s

// P4405: constant evaluation is never analyzed (D4405, notadragon_wg21
// DECISIONS.md E142), so an entry selecting prove or diagnose for constant
// evaluation alone ("constexpr": true) is a configuration error, and is
// skipped.  Here the next entry gives constant evaluation ignore and the run
// time ignore, so a false predicate is not evaluated, the constant
// expression does not fail, and nothing else is diagnosed.  A prove or
// diagnose that constant evaluation reaches otherwise is enforce or observe
// there (p4405-constexpr-enforce.cpp, p4405-constexpr-diagnose.cpp).  Both
// constant evaluators.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-constexpr.C

// expected-error@*:* {{the 'semantic' of an entry matching 'constexpr' cannot be 'prove' or 'diagnose' (entry 0 of}}
// expected-error@*:* {{the 'semantic' of an entry matching 'constexpr' cannot be 'prove' or 'diagnose' (entry 1 of}}

constexpr int f(int x) pre(x > 0) post(r: r > 0)
{
  contract_assert(x > 0);
  return x;
}

static_assert(f(-1) == -1);
constexpr int v = f(-2);
static_assert(v == -2);

int use(int x) { return f(x); }
