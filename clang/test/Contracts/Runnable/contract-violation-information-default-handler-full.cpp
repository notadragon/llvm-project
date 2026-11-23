// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s --match-full-lines --implicit-check-not={{.}}

// Under -fcontract-violation-information=full the default violation handler
// prints the file, line, comment and message, as it always has;
// contract-violation-information-default-handler.cpp is the same under none.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-default-handler-full.C.

void pre_f(int x) pre(x > 0) {}
void msg_f(int x) pre(x > 0, "a user message") {}
void assert_f(int x) { contract_assert(x > 0); }

int main() {
  pre_f(0);
  msg_f(0);
  assert_f(0);
}

// CHECK:      {{.*}}contract-violation-information-default-handler-full.cpp:13: pre(x > 0) failed
// CHECK-NEXT: {{.*}}contract-violation-information-default-handler-full.cpp:14: pre(x > 0) failed: a user message
// CHECK-NEXT: {{.*}}contract-violation-information-default-handler-full.cpp:15: contract_assert(x > 0) failed
