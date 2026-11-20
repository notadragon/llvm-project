// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3098: constructor postconditions with captures, with and without a
// virtual base, compile and check correctly.
//
// GCC's outlined-mode counterpart:
// gcc/testsuite/g++.dg/contracts/cpp26/p3098-ctor-capture-outlined-run.C

static int inits = 0;
int init(int x) { ++inits; return x; }

struct A {
  int v;
  A(int x) post [c = init(x)] (c == 1) : v(x) {}
};

struct V {};
struct B : virtual V {
  int v;
  B(int x) post [c = init(x)] (c == v) : v(x) {}
};

int main() {
  A a(1);
  B b(1);
  if (a.v != 1 || b.v != 1 || inits != 2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
