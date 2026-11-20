// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: a user-defined diagnostic-message on a *contract_assert* statement is
// delivered to the handler when the assertion fails at run time.  p3099-message-
// basic.cpp exercises syntactic parsing for pre/post/contract_assert; this covers
// runtime delivery for the contract_assert form, failing at run time.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-contract-assert.C.)

#include <contracts>
#include <cstring>

static const char* last_message = nullptr;
static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
  ++violations;
}

void f(int x) {
  contract_assert(x > 0, "assert message");
}

int main() {
  f(-1);
  if (violations != 1)
    __builtin_abort();
  if (!last_message || std::strcmp(last_message, "assert message") != 0)
    __builtin_abort();
}
