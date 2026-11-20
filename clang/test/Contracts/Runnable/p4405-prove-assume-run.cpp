// RUN: %clangxx -std=c++26 %s -O2 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -Xclang -Rcontract-proof -Xclang -verify %libcxx_flags -o %t
// RUN: %t

// P4405 -fcontracts-prove-assume: the assumption made of a soundly proven
// evaluation never evaluates its predicate.  positive is declared pure but
// counts its calls, so that a call left of the assumption would show: the
// shadow proves the contract_assert from the test before it (the two calls
// of a pure function are one), soundly, since positive only reads memory,
// returns and does not throw.  Only the test's own call happens.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-assume-run.C

int calls;

[[gnu::pure, gnu::noinline]] bool positive(int x) { ++calls; return x > 0; }

[[gnu::noinline]] int f(int x) {
  if (positive(x)) {
    contract_assert(positive(x));  // expected-remark {{proof shadow: proven, sound; assumed}}
    return x;
  }
  return 0;
}

int main() {
  if (f(5) != 5 || calls != 1)
    __builtin_abort();
  return 0;
}
