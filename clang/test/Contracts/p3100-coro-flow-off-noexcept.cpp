// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -Wno-return-type -fcontract-configuration-file=%S/p3100-coro-flow-off-noexcept.json %libcxx_flags -o %t && %t

// P3100: coroutine flow-off-end with noexcept_observe (-fcontracts-p4298) -- the
// nonthrowing handler runs (assertion_kind::implicit) and control proceeds to
// the final suspend.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-coro-flow-off-noexcept.C)

#include <coroutine>
#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;
static int calls = 0;
static bool ok = true;
void handle_contract_violation(const cs::contract_violation &v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit ||
      v.semantic() != cs::evaluation_semantic::noexcept_observe)
    ok = false;
}

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int) {}
    void unhandled_exception() {}
  };
};

namespace nxo_ns { Task foo(bool b) { if (b) co_return 1; } }   // noexcept_observe

int main() {
  nxo_ns::foo(false);
  if (calls != 1 || !ok) std::abort();
}
