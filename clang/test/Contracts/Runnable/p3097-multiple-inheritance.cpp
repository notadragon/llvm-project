// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Multiple inheritance — correct interface selected per call.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct X1 {
  virtual void f(int n) pre(n > 0) { }
};

struct X2 {
  virtual void f(int n) pre(n > 10) { }
};

struct Y : X1, X2 {
  void f(int n) override pre(n > -100) { }
};

int main() {
  Y y;

  // Call through X1& — only X1 interface contracts.
  violation_count = 0;
  X1& x1 = y;
  x1.f(5);  // X1 pre passes (n>0), Y pre passes (n>-100)
  if (violation_count != 0) __builtin_abort();

  violation_count = 0;
  x1.f(-5);  // X1 pre fails (n>0), Y pre passes (n>-100)
  if (violation_count != 1) __builtin_abort();

  // Call through X2& — only X2 interface contracts.
  violation_count = 0;
  X2& x2 = y;
  x2.f(5);  // X2 pre fails (n>10), Y pre passes (n>-100)
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
