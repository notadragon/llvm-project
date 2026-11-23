// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe \
// RUN:   %libcxx_flags -o %t && env TSAN_OPTIONS=exitcode=0 %t

// TSan contract routing sends only the report kinds that are undefined
// behaviour to the contract-violation handler.  A single-threaded lock-order
// inversion is not one: it stays on the stock path (a warning), and the
// handler is not called.  exitcode=0 keeps the stock warning from failing the
// run.
//
// Mirror: gcc/testsuite/g++.dg/tsan/p3100-tsan-lock-order-not-routed.C

#include <contracts>
#include <mutex>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
}

std::mutex a, b;

int main() {
  {
    std::lock_guard<std::mutex> la(a);
    std::lock_guard<std::mutex> lb(b);
  }
  {
    std::lock_guard<std::mutex> lb(b); // inverted order, one thread
    std::lock_guard<std::mutex> la(a);
  }
  if (calls != 0)
    __builtin_abort();
}
