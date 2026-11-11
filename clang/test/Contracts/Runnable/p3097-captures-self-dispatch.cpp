// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Virtual function with captures called on its own type as well as
// polymorphically.  (Port of GCC
// g++.dg/contracts/cpp26/p3097-captures-self-dispatch.C, which was a regression
// test for a GCC-specific shared-capture-VAR_DECL ICE.)

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int counter = 0;

struct Base {
  virtual int next()
    post [old_c = counter] (r: r == old_c + 1)
  { return ++counter; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int next() override { return counter += 2; }
};

int main() {
  // Polymorphic dispatch (wrapper + callee-side both have captures).
  Derived d;
  Base& b = d;
  counter = 0;
  violation_count = 0;
  b.next();
  if (violation_count != 1) __builtin_abort();  // interface post 2 == 0+1 false

  // Self-dispatch: Base called on its own type.
  Base obj;
  Base& b2 = obj;
  counter = 0;
  violation_count = 0;
  b2.next();
  if (violation_count != 0) __builtin_abort();  // 1 == 0+1 true

  std::printf("PASS\n");
}
