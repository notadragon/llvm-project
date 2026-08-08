// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Deep inheritance — only static and dynamic type contracts checked.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct X {
  virtual void f(int n) pre(n > 0) { }
};

struct Y : X {
  void f(int n) override pre(n > -10) { }
};

struct Z : Y {
  void f(int n) override pre(n > -100) { }
};

int main() {
  Z z;

  // Call through X& — interface is X, implementation is Z.
  // Y's contracts are NOT checked (intermediate).
  violation_count = 0;
  X& x = z;
  x.f(-50);  // X pre fails (n>0), Z pre passes (n>-100)
  if (violation_count != 1) __builtin_abort();

  // Call through Y& — interface is Y, implementation is Z.
  violation_count = 0;
  Y& y = z;
  y.f(-50);  // Y pre fails (n>-10), Z pre passes (n>-100)
  if (violation_count != 1) __builtin_abort();

  // Call through Z& — direct call, only Z contracts.
  violation_count = 0;
  z.f(-50);  // Z pre passes (n>-100)
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
