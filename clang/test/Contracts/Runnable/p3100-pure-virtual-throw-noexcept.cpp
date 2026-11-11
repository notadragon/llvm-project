// P3100: pure-virtual call with a THROWING handler under "enforce", where the
// pure virtual IS declared noexcept.  Because the pure virtual is noexcept, the
// compiler selects the terminate-on-throw terminus
// (__cxa_pure_virtual_noexcept_enforce, reported as sem=7), so a throwing handler
// must std::terminate rather than escaping into a caller that assumed the call
// could not throw -- even though the call site is inside a try/catch.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-configuration-file=%S/p3100-pure-virtual-enforce.json \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s

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
  virtual void f() noexcept = 0; // noexcept: handler must terminate
  void poke();
  virtual ~Base() {}
};
void Base::poke() { f(); }
struct Derived : Base {
  void f() noexcept override {}
};

int main() {
  try {
    Derived d;
    (void)d;
  } catch (Boom &) {
    std::printf("WRONGLY-CAUGHT\n");
    std::fflush(stdout);
  }
  std::printf("WRONGLY-SURVIVED\n");
  return 0;
}

// CHECK: VIOL sem=7
