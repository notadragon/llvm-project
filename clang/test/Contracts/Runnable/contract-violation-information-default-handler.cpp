// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 -fcontracts-p3290 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=none %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s --match-full-lines --implicit-check-not={{.}}

// Under -fcontract-violation-information=none the default violation handler
// prints no empty location parts and no empty comment: no "<file>:0: ", no
// "()", and no message for a contract that had one; a manual violation with
// neither location nor comment says "contract violation".
// contract-violation-information-default-handler-full.cpp is the same under
// full.  libc++'s default handler has its own format ("<file>:<line>:
// pre(<comment>) failed: <message>"), not libstdc++'s, so the lines differ
// from the mirror's while leaving out the same parts.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-default-handler.C.

#include <contracts>

void pre_f(int x) pre(x > 0) {}
void msg_f(int x) pre(x > 0, "a user message") {}
void assert_f(int x) { contract_assert(x > 0); }

int main() {
  pre_f(0);
  msg_f(0);
  assert_f(0);
  std::contracts::handle_observed_contract_violation("");
}

// CHECK:      pre failed
// CHECK-NEXT: pre failed
// CHECK-NEXT: contract_assert failed
// CHECK-NEXT: contract violation
