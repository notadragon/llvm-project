// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: An exception thrown while initializing an interface postcondition
// capture on a virtual call.  This should become a post_capture /
// evaluation_exception contract violation; under observe the handler runs and
// execution continues (the function returns normally).
//
// BUG-2 (Clang, P3097 x P3098, fixed): the capture-init exception used to
// propagate as a raw exception and terminate (uncaught) instead of being
// converted to a contract violation.  See wg21 testing-gap-catalogue.md
// section 10.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Bad {
  Bad(int) { throw 42; }
  Bad(const Bad&) { throw 42; }
  ~Bad() { }
};

struct Base {
  virtual int f(int i)
    post [b = Bad(1)] (true)
  { return i; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f(int i) override { return i; }
};

int main() {
  Derived d;
  Base& b = d;
  violation_count = 0;

  // Correct behavior: post_capture violation reported, execution continues.
  int r = b.f(10);
  if (r != 10) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
