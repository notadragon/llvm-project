// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A generic lambda whose precondition names one of its captures, with a
// deduced return type and with a trailing return type.  With the trailing
// return type, a call instantiates the contracts before the body, which is
// what provides the lambda's scope otherwise.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/generic-lambda-pre-capture.C

int f() {
  int t = 1;
  auto l1 = [t](auto u) pre(u > t) { return u + t; };
  auto l2 = [=](auto u) pre(u > t) { return u + t; };
  return l1(2) + l2(2);
}

int g() {
  int t = 1;
  auto l1 = [t](auto u) -> int pre(u > t) { return u + t; };
  auto l2 = [=](auto u) -> int pre(u > t) post(r : r > t) { return u + t; };
  auto l3 = [s = t](auto u) -> int pre(u > s) { return u + s; };
  auto l4 = [&t](auto u) -> int pre(u > t) { return u + t; };
  return l1(2) + l2(2) + l3(2) + l4(2);
}

template <class T> int h(T t) {
  auto l = [t](auto u) -> int pre(u > t) { return u + t; };
  return l(2);
}
int x = h(1);
