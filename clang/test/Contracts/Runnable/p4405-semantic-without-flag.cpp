// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=prove %libcxx_flags -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t

// P4405: the prove semantic requested on the command line without
// -fcontracts-p4405 is not allowed; the compiler says so and the best fit,
// ignore, is used instead, so nothing is diagnosed and the predicate is not
// evaluated.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-semantic-without-flag.C

static int evaluated;
bool chk(int x) { ++evaluated; return x > 0; }

int f(int x) pre(chk(x)) { return x; }

int main() {
  f(-1);
  return evaluated;
}

// CHECK: warning: '-fcontract-evaluation-semantic=prove' requires '-fcontracts-p4405'; 'ignore' is used instead
// CHECK-NOT: cannot be proven
