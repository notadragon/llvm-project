// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p4405-json-without-flag.json %libcxx_flags -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t

// P4405: a contract configuration asking for prove or diagnose without
// -fcontracts-p4405 is warned about (-Wcontract-configuration), once per
// semantic, and ignore is used instead.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-json-without-flag.C

static int evaluated;
bool chk(int x) { ++evaluated; return x > 0; }

int f(int x) pre(chk(x)) post(r: chk(r)) { contract_assert(chk(x)); return x; }

int main() {
  f(-1);
  return evaluated;
}

// CHECK: warning: the contract configuration asks for 'prove', which requires '-fcontracts-p4405'; 'ignore' is used instead [-Wcontract-configuration]
// CHECK-NEXT: warning: the contract configuration asks for 'diagnose', which requires '-fcontracts-p4405'; 'ignore' is used instead [-Wcontract-configuration]
// CHECK-NOT: warning:
