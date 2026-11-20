// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3097+P3098: Postcondition captures on virtual functions under multiple
// inheritance.  Each base's capturing interface postcondition must bind to the
// correct base subobject across dispatch.  (GCC mirror:
// g++.dg/contracts/cpp26/p3097-captures-multiple-inheritance.C)

#include <contracts>
#include <cstdio>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state_a = 0;
static int state_b = 0;

struct BaseA {
  virtual int fa()
    post [old_a = state_a] (r: r == old_a + 1)
  { return ++state_a; }
  virtual ~BaseA() = default;
};

struct BaseB {
  virtual int fb()
    post [old_b = state_b] (r: r == old_b + 10)
  { return state_b += 10; }
  virtual ~BaseB() = default;
};

struct Derived : BaseA, BaseB {
  int fa() override { return state_a += 2; }   // disagrees with BaseA post (+1)
  int fb() override { return state_b += 10; }  // agrees with BaseB post (+10)
};

int main() {
  Derived d;
  BaseA& ba = d;
  BaseB& bb = d;

  state_a = 0;
  violation_count = 0;
  if (ba.fa() != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  state_b = 0;
  violation_count = 0;
  if (bb.fb() != 10) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
