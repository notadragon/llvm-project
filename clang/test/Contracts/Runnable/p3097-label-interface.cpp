// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3097 x P3400: a label on the INTERFACE (base) contract governs only the
// interface check's semantic.  (GCC mirror: p3097-label-interface.C)

#include <contracts>
#include <cstdio>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

struct Base {
  virtual int f() post<review>(r: r > 100) { return 1; }  // interface: review->observe
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override post(r: r > 0) { return 5; }  // implementation: enforce, passes
};

int main() {
  Derived d;
  Base& b = d;
  handler_count = 0;
  int r = b.f();
  if (r != 5) __builtin_abort();
  if (handler_count != 1) __builtin_abort();
  std::printf("PASS\n");
}
