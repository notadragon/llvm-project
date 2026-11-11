// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-group-evaluation-semantic=g:prove,h:diagnose %libcxx_flags -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t

// P4405: -fcontract-group-evaluation-semantic= asking for prove or diagnose
// without -fcontracts-p4405 is warned about (-Wcontract-configuration), and
// ignore is used instead.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-group-without-flag.C
// (which names the groups with assertion-control labels;
// [[clang::contract_group]] here)

static int evaluated;
bool chk(int x) { ++evaluated; return x > 0; }

int f(int x) pre [[clang::contract_group("g")]] (chk(x)) { return x; }
int g(int x) pre [[clang::contract_group("h")]] (chk(x)) { return x; }

int main() {
  f(-1);
  g(-1);
  return evaluated;
}

// CHECK: warning: the contract configuration asks for 'prove', which requires '-fcontracts-p4405'; 'ignore' is used instead [-Wcontract-configuration]
// CHECK-NEXT: warning: the contract configuration asks for 'diagnose', which requires '-fcontracts-p4405'; 'ignore' is used instead [-Wcontract-configuration]
// CHECK-NOT: warning:
