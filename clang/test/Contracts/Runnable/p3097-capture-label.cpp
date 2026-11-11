// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3097+P3098+P3400: a 'review' label (compute_semantic: enforce -> observe) on
// a capturing interface postcondition of a virtual function.  The capture is
// still constructed and, because review downgrades enforce to observe, the
// violation is reported and execution CONTINUES rather than terminating.
// (GCC mirror: g++.dg/contracts/cpp26/p3097-capture-label.C)

#include <contracts>
#include <cstdio>

static int handler_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

static int state = 0;

struct Base {
  virtual int f()
    post<review> [old = state] (r: r == old + 1)
  { return ++state; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { return state += 2; }  // disagrees with interface post (+1)
};

int main() {
  Derived d;
  Base& b = d;
  state = 0;
  handler_count = 0;

  int r = b.f();
  if (r != 2) __builtin_abort();
  if (handler_count != 1) __builtin_abort();  // reaching here proves no terminate

  std::printf("PASS\n");
}
