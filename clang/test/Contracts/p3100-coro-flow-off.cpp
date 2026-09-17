// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-coro-flow-off.json %libcxx_flags -o %t && %t

// P3100: control flowing off the end of a coroutine whose promise type has no
// usable return_void is core-language UB ({stmt.return.coroutine.flow.off}).
// observe reports (assertion_kind::implicit) then proceeds to the final suspend;
// ignore proceeds without reporting; a co_return avoids the fall-off entirely.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-coro-flow-off.C)

#include <coroutine>
#include <contracts>
#include <cstdlib>
#include <cstring>

namespace cs = std::contracts;
static int calls = 0;
static bool all_implicit = true;
static const char *last_comment = nullptr;
void handle_contract_violation(const cs::contract_violation &v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
  last_comment = v.comment();
}

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int) {}          // has return_value, NO return_void
    void unhandled_exception() {}
  };
};

namespace obs_ns { Task foo(bool b) { if (b) co_return 1; } }   // observe
namespace ign_ns { Task foo(bool b) { if (b) co_return 1; } }   // ignore

int main() {
  obs_ns::foo(false);
  if (calls != 1 || !all_implicit) std::abort();
  if (std::strcmp(last_comment,
                  "control flowed off the end of a coroutine") != 0)
    std::abort();

  obs_ns::foo(true);
  if (calls != 1) std::abort();

  ign_ns::foo(false);
  if (calls != 1) std::abort();
}
