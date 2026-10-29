// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Postcondition captures on a virtual function reached through a
// virtual (diamond) base.  The single shared base subobject's capturing
// interface postcondition is checked once with correct call-time captures.
// (GCC mirror: g++.dg/contracts/cpp26/p3097-captures-virtual-inheritance.C)

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

struct Left : virtual Base { };
struct Right : virtual Base { };

struct Join : Left, Right {
  int f() override { return state += 3; }  // disagrees with Base post (+1)
};

int main() {
  Join j;
  Base& b = j;  // unambiguous: single virtual Base subobject

  state = 0;
  violation_count = 0;
  if (b.f() != 3) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  std::printf("PASS\n");
}
