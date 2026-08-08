// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-coro-flow-off-throw.json %libcxx_flags -o %t && %t

// P3100: a THROWING violation handler for a coroutine flow-off assertion.  The
// fall-off is inside the coroutine's try block, so a throwing observe/enforce
// reaction unwinds the body (destroying in-scope automatic objects) and is
// caught by promise.unhandled_exception ().
// (GCC mirror: g++.dg/contracts/cpp26/p3100-coro-flow-off-throw.C)

#include <coroutine>
#include <contracts>
#include <cstdlib>

struct E {};
static int ueh = 0;
static int dtors = 0;
struct S { ~S() { ++dtors; } };
void handle_contract_violation(const std::contracts::contract_violation &) {
  throw E{};
}

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int) {}
    void unhandled_exception() { ++ueh; }   // catches the thrown reaction
  };
};

namespace obs_ns { Task foo(bool b) { S s; if (b) co_return 1; } }   // observe
namespace enf_ns { Task foo(bool b) { S s; if (b) co_return 1; } }   // enforce

int main() {
  obs_ns::foo(false);
  if (ueh != 1 || dtors != 1) std::abort();
  enf_ns::foo(false);
  if (ueh != 2 || dtors != 2) std::abort();
}
