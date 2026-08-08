// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Devirtualized call (static == dynamic type) — contracts checked once.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Base {
  virtual void f(int x) pre(x > 0) { }
};

struct Derived : Base {
  void f(int x) override pre(x > 0) { }
};

int main() {
  Derived d;

  // Direct call on derived — static == dynamic type.
  // Only Derived's callee-side contracts checked (once).
  violation_count = 0;
  d.f(-1);
  if (violation_count != 1) __builtin_abort();

  // Valid call — no violations.
  violation_count = 0;
  d.f(5);
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
