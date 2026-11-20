// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Interface postcondition with captures across virtual dispatch.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int shared_state = 0;

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post [old_state = shared_state] (r: r == old_state + 1)
  { return shared_state; }
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
  {
    shared_state += x;
    return shared_state;
  }
};

int main() {
  Derived d;
  Base& b = d;

  // shared_state=0, call b.f(1): captures old_state=0, Derived sets to 1.
  // Interface post: 1 == 0+1 -> true.
  shared_state = 0;
  violation_count = 0;
  int r = b.f(1);
  if (violation_count != 0) __builtin_abort();
  if (r != 1) __builtin_abort();

  // shared_state=1, call b.f(5): captures old_state=1, Derived sets to 6.
  // Interface post: 6 == 1+1 -> false (violation).
  violation_count = 0;
  r = b.f(5);
  if (violation_count != 1) __builtin_abort();
  if (r != 6) __builtin_abort();

  std::printf("PASS\n");
}
