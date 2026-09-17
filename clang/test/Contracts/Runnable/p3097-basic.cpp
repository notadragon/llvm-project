// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Basic two-source checking for virtual function contracts.

#include <contracts>
#include <cstdio>

static int check_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++check_count;
}

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0)
  { return x; }
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
    post(r: r > -100)
  { return x; }
};

int main() {
  Derived d;

  // Direct call through derived type: only implementation contracts.
  check_count = 0;
  d.f(5);
  if (check_count != 0) __builtin_abort();

  // Virtual call through base reference with valid input: no violations.
  Base& b = d;
  check_count = 0;
  b.f(5);
  if (check_count != 0) __builtin_abort();

  // Virtual call: interface pre (x>0) fails with x=-50.
  // Implementation pre (x>-100) passes.
  // Derived returns -50, interface post (r>0) also fails.
  check_count = 0;
  b.f(-50);
  if (check_count != 2) __builtin_abort();

  std::printf("PASS\n");
}
