// P3100: pure-virtual call (ub:class.abstract.pure.virtual) configured to
// "enforce".  The slot points at __cxa_pure_virtual_enforce: it reports through
// the handler and then terminates (the enforcing core aborts once the handler
// returns).

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-pure-virtual-enforce.json \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
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

// CHECK: VIOL kind=7 sem=3 comment=[pure virtual function called]
