// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: an empty-string message is distinct from no message.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-empty.C)

#include <contracts>
#include <cstdio>
#include <cstring>

void f(int x) pre(x > 0, "") { }

static bool handler_called = false;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  handler_called = true;
  if (v.message() == nullptr) __builtin_abort();          // empty != nullptr
  if (std::strcmp(v.message(), "") != 0) __builtin_abort(); // is empty string
}

int main() {
  f(-1);
  if (!handler_called) __builtin_abort();
  std::printf("PASS\n");
}
