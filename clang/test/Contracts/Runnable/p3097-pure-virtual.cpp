// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Contracts on pure virtual functions.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Abstract {
  virtual void f(int x) pre(x > 0) = 0;
};

struct Concrete : Abstract {
  void f(int x) override pre(x > -100) { }
};

int main() {
  Concrete c;
  Abstract& a = c;

  violation_count = 0;
  a.f(-1);  // Abstract pre fails (x>0), Concrete pre passes (x>-100)
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  a.f(5);  // both pass
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
