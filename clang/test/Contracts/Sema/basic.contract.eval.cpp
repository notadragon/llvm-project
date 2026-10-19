// RUN: %clang_cc1 -fcontracts -std=c++26 -Wvla -fsyntax-only -verify %s
// RUN: %clang_cc1 -fcontracts -std=c++26 -Wvla -fsyntax-only -verify -fcontract-evaluation-semantic=observe %s
// RUN: %clang_cc1 -fcontracts -std=c++26 -Wvla -fsyntax-only -verify -fcontract-evaluation-semantic=quick_enforce %s
// RUN: %clang_cc1 -fcontracts -std=c++26 -Wvla -fsyntax-only -verify -fcontract-evaluation-semantic=ignore %s
// expected-no-diagnostics

// [basic.contract.eval] - Example #1: the array bound depends on the
// evaluation semantic of the contract_assert.  Under enforce, observe and
// quick_enforce the assertion is evaluated during constant evaluation, but
// what its predicate modifies is then undone (the evaluation without side
// effects the wording permits), so i is not incremented: the bound is 1, as
// under ignore.  Either way the bound is a constant expression, so -Wvla
// stays quiet.
// (GCC mirror: g++.dg/contracts/cpp26/basic-contract-eval-example-enforce.C
// and basic-contract-eval-example-ignore.C)
constexpr int f(int i) {
  contract_assert((++const_cast<int &>(i), true));
  return i;
}
inline void g() {
  int a[f(1)]; // size dependent on the evaluation semantic of contract_assert above
  static_assert(sizeof(a) == 1 * sizeof(int));
}
static_assert(f(1) == 1);
