// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 \
// RUN:   -fcontract-configuration-file=%S/p3097-config-semantic.json %libcxx_flags -o %t && %t

// P3097 x P3595: callee-side configuration selects different semantics for the
// interface (base) and implementation (override) postconditions of a virtual
// function, matched by source location.  The override's post is set to ignore;
// everything else observes.  (GCC mirror: p3097-config-semantic.C)

#include <contracts>
#include <cstdio>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

struct Base {
  virtual int f()
    post(r: r > 100)   // interface post -> observe
  { return 1; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override
    post(r: r > 100)   // implementation post -> ignore (configured range)
  { return 5; }
};

int main() {
  Derived d;
  Base& b = d;
  violations = 0;
  int r = b.f();
  if (r != 5) __builtin_abort();
  if (violations != 1) __builtin_abort();  // impl ignored, interface observed
  std::printf("PASS\n");
}
