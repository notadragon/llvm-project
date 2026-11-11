// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A fold over a pack that a lambda captures, implicitly or explicitly, in
// the lambda's precondition, and by reference in its postcondition.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-pre-fold-capture.C

template <class... Ts> int p(Ts... ts) {
  auto l1 = [=](int u) pre(((u > ts) && ...)) { return (u + ... + ts); };
  auto l2 = [ts...](int u) pre(((u > ts) && ...)) { return (u + ... + ts); };
  return l1(5) + l2(5);
}
int x = p(1, 2);

template <class... Ts> int q(int v, Ts... ts) {
  auto l = [&](int u) pre(((u > ts) && ...)) post(r: ((r > ts) && ...)) {
    return u + (0 * ... * ts);
  };
  // The pack named only in the lambda's own precondition.
  auto m = [ts...](int u) pre(((u > ts) && ...)) { return u; };
  return l(v) + m(v);
}
int y = q(5, 1, 2);
