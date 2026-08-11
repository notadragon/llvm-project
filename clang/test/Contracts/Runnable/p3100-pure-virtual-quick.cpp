// P3100: pure-virtual call (ub:class.abstract.pure.virtual) configured to
// "quick_enforce".  The slot points at __cxa_pure_virtual_quick, a fast silent
// trap -- no handler runs and nothing is printed.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-pure-virtual-quick.json \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

// Must NOT be called under quick_enforce.
void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("HANDLER-SHOULD-NOT-RUN\n");
  std::fflush(stdout);
}

struct Base {
  Base() { poke(); }
  virtual void f() = 0;
  void poke();
  virtual ~Base() {}
};
void Base::poke() { f(); }
struct Derived : Base {
  void f() override {}
};

int main() {
  Derived d;
  (void)d;
  return 0;
}

// CHECK-NOT: HANDLER-SHOULD-NOT-RUN
