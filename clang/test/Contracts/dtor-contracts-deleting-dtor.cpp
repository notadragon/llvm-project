// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A virtual destructor's contracts on `delete p' are evaluated once by the
// destructor itself -- only the variant that runs the user-written body
// evaluates them, not the deleting one that calls it -- and once more by the
// P3097 interface check of the virtual call (DECISIONS.md K8): twice in all.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/dtor-contracts-per-variant.C

static int pres = 0, posts = 0;
bool pre_hit() { ++pres; return true; }
bool post_hit() { ++posts; return true; }

struct S {
  virtual ~S() pre(pre_hit()) post(post_hit()) {}
};

int main() {
  S *p = new S;
  delete p;
  if (pres != 2 || posts != 2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
