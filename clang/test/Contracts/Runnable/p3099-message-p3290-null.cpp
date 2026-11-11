// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 -fcontracts-p3290 \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3099 x P3290: a contract violation triggered through the P3290 library API
// carries no P3099 diagnostic-message -- message() returns nullptr -- while the
// API's comment is delivered independently.  The diagnostic-message accessor and
// the API comment are independent fields (P3099 overview, "Paper Interactions");
// no P3290 API overload supplies a message, so message() is always nullptr for an
// API-triggered violation.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-p3290-null.C.)

#include <contracts>
#include <cstring>

static bool handler_called = false;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  handler_called = true;
  // No source-level diagnostic-message exists for an API-triggered violation.
  if (v.message() != nullptr)
    __builtin_abort();
  // ... but the API's comment is delivered.
  if (!v.comment() || std::strcmp(v.comment(), "api comment") != 0)
    __builtin_abort();
}

int main() {
  std::contracts::handle_observed_contract_violation("api comment");
  if (!handler_called)
    __builtin_abort();
}
