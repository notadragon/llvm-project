// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=shift,signed-integer-overflow \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o0
// RUN: %t.o0 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O1 \
// RUN:   -fsanitize=shift,signed-integer-overflow \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o1
// RUN: %t.o1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=shift,signed-integer-overflow \
// RUN:   -fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o2
// RUN: %t.o2 | FileCheck %s

// P3100: with -fsanitize=shift and signed-integer-overflow routed to the
// contract-violation handler with noexcept_observe, the code after a check
// that reports and continues sees the handler's effects, in the function
// holding the check and in its caller: `reports`, a static only the
// handler changes, is read again after it, at -O0, -O1 and -O2.  The
// forms: a shift and a signed addition in functions that are not inlined,
// and in the caller.  (Inline forms at -O2 alone are in
// p3100-routed-handler-effects.cpp; the whole set of gnu_gcc's forms in
// p3100-routed-handler-effects-builtin.cpp.)
// (GCC mirror: g++.dg/ubsan/p3100-routed-handler-effects-builtin*.C, once
// GCC-858's watch: there the shift check called a __ubsan_handle_*
// function declared a leaf and the signed overflow check was a const
// internal call, fixed in M88-GCC.)

#include <contracts>
#include <cstdio>

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
}

#define NI __attribute__((noinline))
NI int shl(int a, int b) { return a << b; }
NI int add(int a, int b) { return a + b; }

int main() {
  volatile int big = 40, one = 1, mx = __INT_MAX__;
  int before = reports;
  volatile int r = shl(one, big);
  std::printf("shl %d\n", reports - before);
  before = reports;
  r = add(mx, one);
  std::printf("add %d\n", reports - before);
  before = reports;
  r = one << big;
  std::printf("ishl %d\n", reports - before);
  before = reports;
  r = mx + one;
  std::printf("iadd %d\n", reports - before);
  return 0;
}

// CHECK: shl 1
// CHECK-NEXT: add 1
// CHECK-NEXT: ishl 1
// CHECK-NEXT: iadd 1
