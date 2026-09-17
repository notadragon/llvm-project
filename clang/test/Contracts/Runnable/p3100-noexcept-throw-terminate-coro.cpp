// P3100 x P4298: coroutine flow-off-end (ub:stmt.return.coroutine.flow.off)
// with a THROWING handler under noexcept_enforce / noexcept_observe must
// TERMINATE.  Unlike the throwing observe/enforce semantics -- where the
// reaction is thrown inside the coroutine body and caught by
// promise.unhandled_exception() -- the non-throwing noexcept_ semantics
// dispatch through the terminate barrier, so a handler that throws terminates
// the program before the coroutine's try can catch it.  Covered under both
// semantics via per-namespace configuration.
//
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noexcept-throw-terminate-coro.C.)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O0 -g -Wno-return-type \
// RUN:   -fcontract-configuration-file=%S/p3100-noexcept-throw-terminate-coro.json \
// RUN:   %libcxx_flags -o %t
// RUN: %t

#include <coroutine>
#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sys/wait.h>
#include <unistd.h>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation &) {
  throw E{};
}

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(int) {}
    void unhandled_exception() { std::_Exit(3); }  // must NOT be reached
  };
};

static volatile bool never = false;
// Falls off the end (never is false) -> coroutine flow-off UB -> implicit check.
namespace nx_enf { Task foo() { if (never) co_return 1; } }  // noexcept_enforce
namespace nx_obs { Task foo() { if (never) co_return 1; } }  // noexcept_observe

typedef Task (*Fn)();
static int run_terminates(const char *label, Fn fn) {
  pid_t pid = fork();
  if (pid == 0) {
    std::set_terminate([] { std::_Exit(77); });
    try { (void)fn(); } catch (...) { std::_Exit(1); }  // bug: escaped
    std::_Exit(2);                                       // bug: never fired
  }
  int st = 0;
  waitpid(pid, &st, 0);
  bool ok = WIFEXITED(st) && WEXITSTATUS(st) == 77;
  if (!ok)
    std::printf("FAIL %-22s exited=%d code=%d signalled=%d sig=%d\n", label,
                WIFEXITED(st), WIFEXITED(st) ? WEXITSTATUS(st) : -1,
                WIFSIGNALED(st), WIFSIGNALED(st) ? WTERMSIG(st) : -1);
  return ok ? 0 : 1;
}

int main() {
  int bad = 0;
  bad |= run_terminates("noexcept_enforce/coro", nx_enf::foo);
  bad |= run_terminates("noexcept_observe/coro", nx_obs::foo);
  return bad;
}
