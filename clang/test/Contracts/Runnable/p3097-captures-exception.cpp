// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Captures destroyed on exception from virtual dispatch.
// (Port of GCC g++.dg/contracts/cpp26/p3097-captures-exception.C)

#include <contracts>
#include <cstdio>

static int dtor_count = 0;

struct Tracker {
  Tracker() = default;
  Tracker(const Tracker&) { }
  ~Tracker() { ++dtor_count; }
};

void handle_contract_violation(const std::contracts::contract_violation&) { }

static Tracker global_tracker;

struct Base {
  virtual int f()
    post [t = global_tracker] (r: r > 0)
  { return 1; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { throw 42; }
};

int main() {
  Derived d;
  Base& b = d;

  dtor_count = 0;
  try {
    b.f();
    __builtin_abort();  // should not reach here
  } catch (int) {
    // Capture 't' should have been destroyed during unwinding.
  }
  if (dtor_count < 1) __builtin_abort();

  std::printf("PASS\n");
}
