// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// An await-expression or a yield-expression may appear in a contract
// predicate only inside a lambda-expression in it ([expr.await], P2900):
// beyond coroutine-compat.cpp's plain forms, the bare operand, a
// parenthesized co_yield, one inside a statement expression, one as a comma
// operand, one in a coroutine's precondition and a co_yield in its
// postcondition are errors, and a lambda that is itself a coroutine is fine.
// (From the EDG fork's sema/predicate_await, EDG-59.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/await-in-predicate.C (GCC-643).

#include <coroutine>

struct A {
  bool await_ready() { return true; }
  void await_suspend(std::coroutine_handle<>) {}
  bool await_resume() { return true; }
};
struct task {
  struct promise_type {
    task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    A yield_value(int) { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};

task c1() { contract_assert(co_await A{}); co_return; } // expected-error {{'co_await' cannot be used inside a contract}}
task c2() { contract_assert((co_yield 1)); co_return; } // expected-error {{'co_yield' cannot be used inside a contract}}
task c3() {
  contract_assert(({ co_await A{}; })); // expected-error {{'co_await' cannot be used inside a contract}} expected-error {{not contextually convertible to 'bool'}}
  co_return;
}
task c4() {
  contract_assert(([]() -> task { co_await A{}; co_return; }(), true)); // OK
  co_return;
}
task c5() pre((co_await A{}, true)) { co_return; } // expected-error {{'co_await' cannot be used inside a contract}}
task c6(int) post(((co_yield 1), true)) { co_return; } // expected-error {{'co_yield' cannot be used inside a contract}}
task c7() { contract_assert((co_await A{}, true)); co_return; } // expected-error {{'co_await' cannot be used inside a contract}}
