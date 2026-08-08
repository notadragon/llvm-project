// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Qualified call bypasses virtual dispatch — no wrapper.

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
  void f(int x) override pre(x > 10) { }
};

int main() {
  Derived d;

  // Qualified call: d.Base::f(-1)
  // Only Base's callee-side contracts checked (no virtual dispatch wrapper).
  violation_count = 0;
  d.Base::f(-1);
  if (violation_count != 1) __builtin_abort();  // Base pre (x>0) fails

  // d.Base::f(5) passes Base pre (x>0), no violations.
  violation_count = 0;
  d.Base::f(5);
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
