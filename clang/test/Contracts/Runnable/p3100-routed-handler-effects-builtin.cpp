// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=shift,signed-integer-overflow,integer-divide-by-zero \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe,integer-divide-by-zero:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o0
// RUN: %t.o0 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O1 \
// RUN:   -fsanitize=shift,signed-integer-overflow,integer-divide-by-zero \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe,integer-divide-by-zero:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o1
// RUN: %t.o1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=shift,signed-integer-overflow,integer-divide-by-zero \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe,integer-divide-by-zero:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o2
// RUN: %t.o2 | FileCheck %s

// P3100: with -fsanitize=shift, signed-integer-overflow and
// integer-divide-by-zero routed to the contract-violation handler with
// noexcept_observe, the code after a check that reports and continues sees
// the handler's effects, in the function holding the check and in its
// caller: `reports`, a static only the handler changes, is read again
// after it, at -O0, -O1 and -O2.  The forms: a shift, a signed addition,
// subtraction, multiplication and negation, a division overflowing and a
// division by zero, in functions that are not inlined (the caller reads
// `reports` after the call) and in the caller itself.  A division, which
// traps after the report, is caught as SIGFPE and resumed with siglongjmp.
// (An LTO run, as gnu_gcc's, waits on CLANG-798.)
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin*.C
// (GCC-858), whose header p3100-routed-handler-effects-builtin.h has these
// forms in this order.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
}

static sigjmp_buf env;
static void on_fpe(int) { siglongjmp(env, 1); }

#define NI __attribute__((noinline))
NI int shl(int a, int b) { return a << b; }
NI int add(int a, int b) { return a + b; }
NI int sub(int a, int b) { return a - b; }
NI int mul(int a, int b) { return a * b; }
NI int neg(int a) { return -a; }
NI int dvo(int a, int b) { return a / b; }
NI int dvz(int a, int b) { return a % b; }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

#define COUNT_TRAP(name, ...)                                                  \
  do {                                                                         \
    int before = reports;                                                      \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  volatile int big = 40, one = 1, mx = __INT_MAX__, mn = -__INT_MAX__ - 1;
  volatile int m1 = -1, zero = 0;
  volatile int r;
  std::signal(SIGFPE, on_fpe);
  COUNT("shl", r = shl(one, big));
  COUNT("add", r = add(mx, one));
  COUNT("sub", r = sub(mn, one));
  COUNT("mul", r = mul(mx, mx));
  COUNT("neg", r = neg(mn));
  COUNT_TRAP("dvo", r = dvo(mn, m1));
  COUNT_TRAP("dvz", r = dvz(one, zero));
  COUNT("ishl", r = one << big);
  COUNT("iadd", r = mx + one);
  COUNT("isub", r = mn - one);
  COUNT("imul", r = mx * mx);
  COUNT("ineg", r = -mn);
  COUNT_TRAP("idvo", r = mn / m1);
  COUNT_TRAP("idvz", r = one % zero);
  (void)r;
  return 0;
}

// CHECK: shl 1
// CHECK-NEXT: add 1
// CHECK-NEXT: sub 1
// CHECK-NEXT: mul 1
// CHECK-NEXT: neg 1
// CHECK-NEXT: dvo 1
// CHECK-NEXT: dvz 1
// CHECK-NEXT: ishl 1
// CHECK-NEXT: iadd 1
// CHECK-NEXT: isub 1
// CHECK-NEXT: imul 1
// CHECK-NEXT: ineg 1
// CHECK-NEXT: idvo 1
// CHECK-NEXT: idvz 1
