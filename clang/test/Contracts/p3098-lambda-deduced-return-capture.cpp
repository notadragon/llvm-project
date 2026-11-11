// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3098: a lambda with a deduced return type and a postcondition with a
// capture evaluates the capture and the predicate.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-lambda-capture-run.C

static int inits = 0;
int init() { ++inits; return 1; }

int main() {
  auto lam = [](int x) post [c = init()] (c == 1) { return x; };
  if (lam(7) != 7 || inits != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
