// P3100: a call that dispatches to a pure virtual function
// (ub:class.abstract.pure.virtual) configured to "observe".  The pure virtual's
// vtable slot is pointed at __cxa_pure_virtual_observe, which reports through the
// contract-violation handler as an implicit (assertion_kind::implicit) assertion
// and then terminates -- a pure-virtual call has no valid continuation.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-pure-virtual-observe.json \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}

struct Base {
  Base() { poke(); } // dispatches to the pure virtual during construction
  virtual void f() = 0;
  void poke();
  virtual ~Base() {}
};
// Out-of-line so the call is a genuine vtable dispatch (no devirtualization).
void Base::poke() { f(); }
struct Derived : Base {
  void f() override {}
};

int main() {
  Derived d;
  (void)d;
  return 0;
}

// CHECK: VIOL kind=7 sem=2 comment=[pure virtual function called]
