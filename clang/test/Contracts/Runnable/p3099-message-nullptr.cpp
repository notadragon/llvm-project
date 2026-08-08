// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: message() is nullptr when no message is supplied; comment() still works.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-nullptr.C)
//
// (Previously BUG-10: for a contract with NO message, message() returned a
// non-null empty string, indistinguishable from an explicit empty message.
// BuildViolationObject now emits a null __message_ pointer unless the contract
// actually carries a message -- syntactic, transformed (P2741), or attribute.)

#include <contracts>
#include <cstdio>

void f(int x) pre(x > 0) { }

static bool handler_called = false;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  handler_called = true;
  if (v.message() != nullptr) __builtin_abort();
  if (v.comment() == nullptr) __builtin_abort();
}

int main() {
  f(-1);
  if (!handler_called) __builtin_abort();
  std::printf("PASS\n");
}
