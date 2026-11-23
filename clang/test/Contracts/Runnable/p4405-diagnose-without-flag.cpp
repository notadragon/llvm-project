// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=diagnose %libcxx_flags -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t

// P4405: likewise for the diagnose semantic: without -fcontracts-p4405 it
// is not allowed, the compiler says so, and ignore is used instead.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-diagnose-without-flag.C

static int evaluated;
bool chk(int x) { ++evaluated; return x > 0; }

int f(int x) pre(chk(x)) post(r: chk(r)) { contract_assert(chk(x)); return x; }

int main() {
  f(-1);
  return evaluated;
}

// CHECK: warning: '-fcontract-evaluation-semantic=diagnose' requires '-fcontracts-p4405'; 'ignore' is used instead
// CHECK-NOT: cannot be proven
