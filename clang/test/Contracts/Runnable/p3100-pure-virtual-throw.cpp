// P3100: pure-virtual call with a THROWING contract-violation handler under
// "enforce", where the pure virtual is NOT noexcept.  The
// __cxa_pure_virtual_enforce terminus lets the handler's exception unwind out
// through the (failed) construction to the caller's try/catch, so execution
// resumes there.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-pure-virtual-enforce.json \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

struct Boom {};

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}

struct Base {
  Base() { poke(); }
  virtual void f() = 0; // not noexcept: handler may propagate
  void poke();
  virtual ~Base() {}
};
void Base::poke() { f(); }
struct Derived : Base {
  void f() override {}
};

int main() {
  try {
    Derived d;
    (void)d;
  } catch (Boom &) {
    std::printf("CAUGHT\n");
    std::fflush(stdout);
  }
  std::printf("SURVIVED\n");
  return 0;
}

// CHECK: VIOL sem=3
// CHECK: CAUGHT
// CHECK: SURVIVED
