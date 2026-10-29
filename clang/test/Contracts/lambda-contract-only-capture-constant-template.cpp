// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// A constant odr-used only in a contract assertion of a lambda in a template
// is captured only for the contract assertion, which P2900
// [expr.prim.lambda.closure] makes ill-formed; one only read is not
// captured at all.  One odr-used outside the contract assertion too is
// captured for both and is not diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-constant-template.C
// in the gnu_gcc fork (GCC-653).

bool nonnull(const int *p) { return p; }

template <class T>
void g() {
  const int n = 5;
  auto l1 = [=] { contract_assert(n > 0); };          // OK
  auto l2 = [=] { contract_assert(nonnull(&n)); };    // expected-error {{local entity 'n' is used only in contract assertions and must be captured explicitly}}
  auto l3 = [=] { const int *p = &n; contract_assert(&n == p); }; // OK
  l1(); l2(); l3();
}
template void g<int>();
