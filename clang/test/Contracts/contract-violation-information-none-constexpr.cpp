// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-violation-information=none -fsyntax-only -verify %s

// -fcontract-violation-information=none leaves compile-time diagnostics
// alone: a contract violated during constant evaluation is still diagnosed
// at the assertion, with its user message.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-none-constexpr.C.

constexpr int pre_f(int x)
  pre(x > 0, "pre message") // expected-error {{contract failed during execution of constexpr function: pre message}}
{
  return x;
}

constexpr int assert_f(int x) {
  contract_assert(x > 0, "assert message"); // expected-error {{contract failed during execution of constexpr function: assert message}}
  return x;
}

constexpr int a = pre_f(0); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'pre_f(0)'}}
constexpr int b = assert_f(0); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'assert_f(0)'}}
