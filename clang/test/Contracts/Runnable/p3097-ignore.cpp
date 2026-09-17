// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t

// P3097: Ignore semantic — no wrapper, no contract checks.

#include <cstdio>
#include <cassert>

static int call_count = 0;

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0)
  { return x; }
};

struct Derived : Base {
  int f(int x) override pre(x > -100) post(r: r > -100)
  {
    ++call_count;
    return x;
  }
};

int main() {
  Derived d;
  Base& b = d;

  // Under ignore: no contract checks at all, but function still callable.
  call_count = 0;
  int r = b.f(-50);
  assert(r == -50);
  assert(call_count == 1);

  std::printf("PASS\n");
}
