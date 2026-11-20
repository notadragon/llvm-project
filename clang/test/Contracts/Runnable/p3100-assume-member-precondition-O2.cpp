// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -O2 %libcxx_flags -o %t
// RUN: %t

// P3100: the assume semantic on a precondition of an inline member function
// that reads a data member (`this->v`) beside a parameter, at -O2.  Clang
// lowers the assume semantic to ignore, so this compiles and the call
// returns its value.
//
// GCC mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-assume-ipa-sra-split-ice.C

[[gnu::noinline]] bool check(int i) { return i > 0; }

struct S {
  int v = 1;
  [[gnu::noinline]] int m(int i) pre(check(i + v)) { return i + v; }
};

int main() {
  S s;
  return s.m(2) != 3;
}
