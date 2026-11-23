// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify -verify-ignore-unexpected=note %s

// Under observe, each contract problem is warned about once, however many
// times the implementation evaluates the expression -- a static_assert is
// evaluated more than once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-observe-warning-once.C

constexpr int f(const int &v)
  pre(false) // expected-warning {{contract failed}}
{
  return 1;
}

static_assert(f(1) == 1);

// Here the first precondition is not constant (it reads g) and the second is
// false; each is still warned about once.
constexpr int h(const int &v)
  pre(v > 0) // expected-warning {{contract failed}}
  pre(false) // expected-warning {{contract failed}}
{
  return 1;
}

int g = 1;
static_assert(h(g) == 1);
