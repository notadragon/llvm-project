// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// A constant that a lambda's contract assertion names without odr-using it
// is not captured at all ([basic.def.odr], [expr.prim.lambda.capture]), so
// the rule against implicit captures made only for contract assertions
// (P2900 [expr.prim.lambda.closure]) does not apply, with or without a
// capture-default.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-constant.C
// in the gnu_gcc fork.  Found by the EDG fork's sema/contract_only_capture
// test.

bool nonnull(const int *p) { return p; }

void f() {
  const int n = 5;
  constexpr int c = 6;
  auto l1 = [=] { contract_assert(n > 0); };
  auto l2 = [&] { contract_assert(n > 0); };
  auto l3 = [=] { contract_assert(c > 0); };
  auto l4 = [] { contract_assert(n > 0 && c > 0); };
  auto l5 = [=] pre(n > 0) {};
  // An odr-use is a capture, and is diagnosed.
  auto l6 = [=] { contract_assert(nonnull(&n)); }; // expected-error {{local entity 'n' is used only in contract assertions and must be captured explicitly}}
  l1(); l2(); l3(); l4(); l5(); l6();
}

template <class T>
void g() {
  const int n = 5;
  auto l1 = [=] { contract_assert(n > 0); };
  auto l2 = [=] pre(n > 0) {};
  l1(); l2();
}
template void g<int>();
