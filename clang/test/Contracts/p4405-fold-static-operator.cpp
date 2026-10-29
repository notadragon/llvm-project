// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405: a static operator() is called with its object as the first
// argument of the call, evaluated and ignored, so the arguments the
// precondition's parameters are given at a call start after it, for
// folding (-O0) and for the proof shadow (-O2) alike.  CLANG-854: folding
// gave each parameter the argument before its own, so s(1, -1) was proven
// with b == 1.  Also a static lambda, whose predicate names no capture, and
// a lambda: their preconditions are proven at a call from the arguments.
//
// Mirror: none (Clang's own argument mapping).

struct S {
  static int operator()(int a, int b) pre(b > 0); // expected-note {{the predicate of the contract assertion}}
};

int use(S s) {
  int r = s(1, -1); // expected-error {{contract predicate is always false}}
  r += s(-1, 1);
  auto l = [](int a, int b) static pre(b > 0) { return a + b; }; // expected-note {{the predicate of the contract assertion}}
  r += l(-1, 1);
  r += l(1, -1); // expected-error {{contract predicate is always false}}
  auto m = [](int a) pre(a > 0) { return a; };
  r += m(1);
  return r;
}
