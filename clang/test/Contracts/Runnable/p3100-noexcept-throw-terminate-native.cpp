// P3100 x P4298: a THROWING contract-violation handler under the non-throwing
// evaluation semantics noexcept_enforce / noexcept_observe must TERMINATE the
// program for every built-in implicit check -- the exception must never escape
// (P4298: these semantics are non-throwing; p3100-check-table.md "confirm the
// non-throwing variants ... do NOT propagate").  These are the NATIVE
// (front-end / middle-end synthesized) checks, which dispatch through the
// _noexcept terminate barrier directly (not the sanitizer report path).
//
// Covers the front-end-synthesized checks (shift, int / and % by zero,
// INT_MIN/-1 div overflow, float->int cast, array-bounds, flow-off) and the
// three middle-end checks (signed overflow, invalid enum/bool load,
// null-deref), under BOTH noexcept_enforce (namespace nx_enf) and
// noexcept_observe (namespace nx_obs), selected per-namespace by the config
// file.  (Coroutine flow-off is in the companion -coro test.)
//
// Each check runs in a forked child that installs a set_terminate marker
// exiting 77; the child wraps the trigger in try/catch so an exception that
// escapes the barrier (the bug) is caught and exits 1 instead, and a check that
// never fires exits 2.  The parent asserts every child exited via the terminate
// marker (77): that distinguishes a correct terminate from an escaped-and-caught
// exception or a raw crash.  main returns 0 (success) only if all pass.
//
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noexcept-throw-terminate-native.C.)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O0 -g -Wno-return-type \
// RUN:   -fcontract-configuration-file=%S/p3100-noexcept-throw-terminate-native.json \
// RUN:   %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <climits>
#include <sys/wait.h>
#include <unistd.h>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation &) {
  throw E{};                     // must never escape under a noexcept_ semantic
}

// Runtime-fed operands defeat constant folding (a constant-expression trigger
// would be a compile error) so every check is a real run-time violation.
static volatile int vz = 0, vbig = 100, vimin = INT_MIN, vm1 = -1, vmax = INT_MAX;
static volatile double vhuge = 1e300;
enum E3 { A0, A1, A2 };          // valid range 0..3

#define CHECKS                                                              \
  C(shift,    return 1 << vbig;)              /* expr.shift.neg.and.width  */ \
  C(divzero,  return 7 / vz;)                 /* expr.mul.div.by.zero (/)  */ \
  C(remzero,  return 7 % vz;)                 /* expr.mul.div.by.zero (%)  */ \
  C(divovf,   return vimin / vm1;)            /* expr.mul.representable    */ \
  C(fpcast,   return (int)vhuge;)             /* conv.fpint.float.not.repr */ \
  C(bounds,   int a[4] = {}; return a[vbig];) /* expr.add.out.of.bounds    */ \
  C(flowoff,  if (vz > 999999) return 1;)     /* stmt.return.flow.off      */ \
  C(overflow, return vmax + vbig;)            /* signed.integer (mid-end)  */ \
  C(enumload, E3 e; *(int *)&e = 7; return e ? 1 : 0;) /* enum (mid-end)   */ \
  C(nullderef,int *p = (int *)(long)vz; return *p;)    /* nullptr (mid-end)*/

#define C(name, body) __attribute__((noinline)) static int name() { body }
namespace nx_enf { CHECKS }
#undef C
#define C(name, body) __attribute__((noinline)) static int name() { body }
namespace nx_obs { CHECKS }
#undef C

typedef int (*Fn)();
static int run_terminates(const char *label, Fn fn) {
  pid_t pid = fork();
  if (pid == 0) {
    std::set_terminate([] { std::_Exit(77); });      // correct: terminate
    try { (void)fn(); } catch (...) { std::_Exit(1); } // bug: escaped
    std::_Exit(2);                                     // bug: check never fired
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
#define C(name, body)                                                       \
  bad |= run_terminates("noexcept_enforce/" #name, nx_enf::name);           \
  bad |= run_terminates("noexcept_observe/" #name, nx_obs::name);
  CHECKS
#undef C
  return bad;                    // 0 => every check terminated as required
}
