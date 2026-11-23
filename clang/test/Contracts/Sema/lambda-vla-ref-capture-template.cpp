// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -Wno-vla-cxx-extension %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -emit-llvm-only -verify -Wno-vla-cxx-extension %s
// expected-no-diagnostics

// A VLA captured by reference in a lambda in a function template, with the
// template instantiated, in C++26 and with no contract anywhere.  GCC crashed
// on this shape at instantiation (PR127911, an internal compiler error in
// set_contract_capture_flag): the instantiation cleared a contract-only mark
// on the VLA's capture proxy, whose value expression rebuilds the array from
// a {pointer, bound} capture rather than naming the closure member.  Contract
// capture bookkeeping must leave such a capture alone; here all three forms
// are valid and accepted.  GCC has contracts on by default in C++26; Clang
// does not, so the second RUN line turns them on (and code-generates).
//
// GCC mirror: g++.dg/contracts/cpp26/pr127911.C

typedef __INTPTR_TYPE__ intptr_t;

// The report's case: explicit by-reference capture, the VLA's type named in
// the lambda body.
template <typename T> void foo(T n) {
  intptr_t buf[n];
  [&buf]() { __typeof(buf) baz; }();
}

void bar() { foo(42); }

// The capture read and written through, alongside a by-copy capture.
template <typename T> int sum(T n) {
  int buf[n];
  for (T i = 0; i < n; ++i)
    buf[i] = i;
  return [&buf, n]() {
    int s = 0;
    for (T i = 0; i < n; ++i)
      s += buf[i];
    return s;
  }();
}

int baz() { return sum(4); }

// The VLA captured implicitly, by a capture-default.
template <typename T> void bar2(T n) {
  int buf[n];
  [&]() { __typeof(buf) qux; (void)buf[0]; }();
}

void quux() { bar2(3); }
