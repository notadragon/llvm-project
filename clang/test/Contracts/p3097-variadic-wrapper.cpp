// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// Under P3097, a virtual call to a C-variadic function is checked at the call
// site rather than through a wrapper, which could not pass the variadic
// arguments on (DECISIONS.md K12); the arguments reach the final overrider.
// p3097-variadic-call-site.cpp checks the contracts themselves.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3097-variadic-wrapper.C

#include <cstdarg>

struct B {
  virtual int sum(int n, ...) pre(n >= 0);
};

int B::sum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i < n; ++i)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}

struct D : B {
  int sum(int n, ...) override {
    va_list ap;
    va_start(ap, n);
    int s = 100;
    for (int i = 0; i < n; ++i)
      s += va_arg(ap, int);
    va_end(ap);
    return s;
  }
};

[[gnu::noinline]] int call(B &r) { return r.sum(3, 1, 2, 3); }

int main() {
  B b;
  D d;
  if (call(b) != 6 || call(d) != 106)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
