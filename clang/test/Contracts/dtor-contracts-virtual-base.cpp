// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// Each assertion is evaluated once for a destructor's contracts in a class
// with a virtual base (the complete variant delegates to the base one): only
// the variant that runs the user-written body evaluates the contracts.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/dtor-contracts-per-variant.C

static int pres = 0, posts = 0, ctor_pres = 0;
bool pre_hit() { ++pres; return true; }
bool post_hit() { ++posts; return true; }
bool ctor_pre_hit() { ++ctor_pres; return true; }

struct V {};
struct S : virtual V {
  S() pre(ctor_pre_hit()) {}
  ~S() pre(pre_hit()) post(post_hit()) {}
};

int main() {
  { S s; }
  if (ctor_pres != 1 || pres != 1 || posts != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
