// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// When a postcondition capture initializer throws, the captures of the same
// postcondition that were already initialized are destroyed.  Under observe
// the post_capture violation is reported and execution continues.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/capture-partial-leak.C

static int live = 0;
struct Good {
  Good() { ++live; }
  Good(const Good &) { ++live; }
  ~Good() { --live; }
};
struct Bad {
  Bad() { throw 42; }
};

int f(int i) post [g = Good(), b = Bad()] (true) { return i; }

int main() {
  f(1);
  if (live != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
