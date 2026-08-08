// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: A qualified (non-polymorphic) call to a capturing virtual
// function bypasses dispatch, so only the named level's captures/postcondition
// run.  (GCC mirror: g++.dg/contracts/cpp26/p3097-captures-qualified.C)

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state = 0;

struct Base {
  virtual int f()
    post [old = state] (r: r == old + 1)
  { return ++state; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override
    post [old = state] (r: r == old + 2)
  { return state += 2; }
};

int main() {
  Derived d;
  Base& b = d;

  // Qualified call d.Base::f(): only Base::f runs, only Base's post checked.
  state = 0;
  violation_count = 0;
  if (d.Base::f() != 1) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  // Polymorphic call b.f(): both implementation and interface posts run.
  violation_count = 0;
  if (b.f() != 3) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
