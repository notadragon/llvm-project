// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: the interface postcondition capture of a virtual function
// captures the correct value across dispatch (non-zero, so an uninitialized
// capture reading 0 would be detected).
// (GCC mirror: g++.dg/contracts/cpp26/p3097-capture-value.C.)

#include <contracts>

static int shared = 100;
static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++viol; }

struct Base {
  virtual int f() post [old = shared] (r: r == old + 1) { return shared; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { shared += 1; return shared; }
};

int main() {
  Derived d;
  Base& b = d;
  viol = 0;
  int r = b.f();          // old = 100; Derived returns 101; 101 == 100 + 1 holds
  if (r != 101) __builtin_abort();
  if (viol != 0) __builtin_abort();

  int r2 = b.f();         // old = 101; Derived returns 102; 102 == 101 + 1 holds
  if (r2 != 102) __builtin_abort();
  if (viol != 0) __builtin_abort();
}
