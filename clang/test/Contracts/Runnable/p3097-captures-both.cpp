// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Both interface and implementation postconditions have captures.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int counter = 0;

struct Base {
  virtual int f()
    post [old_c = counter] (r: r == old_c + 1)
  { return ++counter; }
};

struct Derived : Base {
  int f() override
    post [old_c = counter] (r: r == old_c + 2)
  {
    counter += 2;
    return counter;
  }
};

int main() {
  Derived d;
  Base& b = d;

  // counter=0. Interface captures old_c=0. Impl captures old_c=0.
  // Derived::f: counter=2, returns 2.
  // Impl post: 2 == 0+2 -> true.
  // Interface post: 2 == 0+1 -> false (violation).
  counter = 0;
  violation_count = 0;
  int r = b.f();
  if (r != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
