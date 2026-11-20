// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// When a violation handler throws from a postcondition with a capture, the
// already initialized returned object is destroyed ([except.ctor]/2).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/retval-leak-throwing-post.C

#include <contracts>

static int ctor = 0, dtor = 0;
struct R {
  R() { ++ctor; }
  R(const R &) { ++ctor; }
  ~R() { ++dtor; }
};

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 7;
}

R f(int x) post [c = x] (c < 0) { return R(); }

int main() {
  try { R r = f(1); } catch (int) {}
  if (ctor != 1 || dtor != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
