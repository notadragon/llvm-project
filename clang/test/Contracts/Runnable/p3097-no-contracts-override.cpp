// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097: Override with no contracts — interface contracts still checked.

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0) { return x; }
};

struct Derived : Base {
  int f(int x) override { return x; }  // no contracts
};

int main() {
  Derived d;
  Base& b = d;

  violation_count = 0;
  b.f(5);
  if (violation_count != 0) __builtin_abort();  // all pass

  violation_count = 0;
  b.f(-1);
  // Interface pre (x>0) fails.  No implementation pre.
  // Implementation returns -1, interface post (r>0) fails.
  if (violation_count != 2) __builtin_abort();

  std::printf("PASS\n");
}
