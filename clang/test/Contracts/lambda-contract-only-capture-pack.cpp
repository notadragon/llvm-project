// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// A function parameter pack odr-used only in a lambda's contract assertions
// is not implicitly captured (P2900 [expr.prim.lambda.closure]), as for a
// single parameter.  Clang reports it once per element of the pack.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-pack.C
// in the gnu_gcc fork.  Found by the EDG fork's sema/contract_only_capture
// test.

template <class... Ts>
void p(Ts... ts) {
  auto l = [=] { contract_assert(((ts > 0) && ...)); };  // expected-error 2 {{local entity 'ts' is used only in contract assertions and must be captured explicitly}}
  auto m = [&] { contract_assert(((ts > 0) && ...)); };  // expected-error 2 {{local entity 'ts' is used only in contract assertions and must be captured explicitly}}
  auto n = [=] { contract_assert(((ts > 0) && ...)); return (ts + ...); };  // OK, also used outside
  auto o = [ts...] { contract_assert(((ts > 0) && ...)); };  // OK, explicit
  l(); m(); n(); o();
}
template void p(int, int);  // expected-note {{in instantiation of}}

void q(int t) {
  auto l = [=] { contract_assert(t > 0); };  // expected-error {{local entity 't' is used only in contract assertions and must be captured explicitly}}
  l();
}

// Diagnosed once for a capture that is not a pack.
template <class T>
void r(T t) {
  auto l = [=] { contract_assert(t > 0); };  // expected-error {{local entity 't' is used only in contract assertions and must be captured explicitly}}
  l();
}
template void r(int);
