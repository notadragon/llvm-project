// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-coro-flow-off-quick.json %libcxx_flags -o %t && not --crash %t

// P3100: coroutine flow-off-end with quick_enforce -- traps at the fall-off.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-coro-flow-off-quick.C)

#include <coroutine>

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int) {}
    void unhandled_exception() {}
  };
};

Task foo(bool b) { if (b) co_return 1; }

int main() { foo(false); return 0; }
