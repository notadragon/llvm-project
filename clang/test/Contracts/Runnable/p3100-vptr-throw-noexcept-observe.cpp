// P3100 x P4298 sanitizer-routed path (regression): a THROWING contract-
// violation handler under noexcept_observe must TERMINATE the program, not
// escape into the sanitizer runtime's non-unwindable C frames.  Routes the
// UBSan vptr check to the handler and has the handler throw; the routed-path
// terminate barrier (__contract_dispatch_core_noexcept, reached via
// __cxa_contract_violation_sanitizer) must catch the throw and call
// std::terminate.  Even though observe would normally continue after the
// handler returns, a handler that THROWS under the noexcept_ semantics must not
// escape -- it terminates at the barrier.
//
// LINKAGE: see the -enforce companion; the weak ref in libcontracts.so resolves
// to libc++'s definition at load time (libc++ DT_NEEDEDs libcontracts).
//
// (GCC mirror: g++.dg/ubsan/p3100-vptr-throw-noexcept-observe.C.)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
  throw E{};                     // must be caught by the barrier -> terminate
}

struct S { S() : a(0) {} virtual int v() { return 0; } int a; };
struct T : S { T() : b(0) {} int b; };
__attribute__((noinline)) static int access_b(T *p) { return p->b; }

int main() {
  std::set_terminate([] { std::printf("TERMINATED\n"); std::fflush(stdout);
                          std::_Exit(0); });
  try {
    S s;
    T *p = reinterpret_cast<T *>(&s);      // vptr: *p is really an S, not a T
    volatile int sink = access_b(p);
    (void)sink;
    std::printf("survived\n");             // bug: exception did not terminate
  } catch (...) {
    std::printf("ESCAPED\n");              // bug: exception escaped the barrier
  }
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: TERMINATED
// CHECK-NOT: ESCAPED
// CHECK-NOT: survived
