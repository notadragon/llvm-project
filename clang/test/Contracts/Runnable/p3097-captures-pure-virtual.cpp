// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Postcondition captures on a pure virtual function with an
// out-of-line definition.  The interface capturing postcondition must be checked
// with correct call-time captures on both polymorphic dispatch and a qualified
// call to the pure-virtual's own definition.
//
// (Previously: the capturing postcondition on a pure virtual with an
// out-of-line definition read 'old' as uninitialized storage, producing a
// spurious violation on the qualified call.  The real cause was general to any
// declaration/definition split: RebuildContractSpecifierForDecl copied the
// capture to the definition without its initializer.  Fixed by rebuilding the
// capture initializer against the definition's parameters.)

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state = 0;

struct Base {
  virtual int f()
    post [old = state] (r: r == old + 1) = 0;
  virtual ~Base() = default;
};

int Base::f() { return ++state; }

struct Derived : Base {
  int f() override { return state += 2; }  // disagrees with interface post (+1)
};

int main() {
  Derived d;
  Base& b = d;

  // Polymorphic dispatch: captures old = 0, Derived sets state = 2 -> violation.
  state = 0;
  violation_count = 0;
  if (b.f() != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  // Qualified call to the definition: captures old = 2, state -> 3;
  // post 3 == 2+1 -> true -> no violation.
  violation_count = 0;
  if (b.Base::f() != 3) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
