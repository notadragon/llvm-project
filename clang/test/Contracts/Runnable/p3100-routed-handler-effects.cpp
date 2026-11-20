// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 -O2 -w \
// RUN:   -fsanitize=array-bounds,alignment,signed-integer-overflow,shift \
// RUN:   -fsanitize-semantic=array-bounds:noexcept_observe,alignment:noexcept_observe,signed-integer-overflow:noexcept_observe,shift:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// P3100: the effects of the contract-violation handler reached from a
// sanitizer check routed to it are seen by the code after the check when
// optimizing: the front end emits the check's handler call, which the
// optimizer treats like any other call, so `reports`, a static only the
// handler changes, is read again after it.  Each form is counted once at
// -O2.  (The checks the middle end inserts after optimizing --
// local-bounds, address, memory, thread -- do not see it: CLANG-761,
// OpenBugs/p3100-routed-handler-effects-late.cpp.  So array-bounds here,
// not the bounds group, which includes local-bounds.)
// (GCC mirror: g++.dg/ubsan/p3100-routed-handler-effects.C, GCC-805,
// bounds and alignment.)

#include <climits>
#include <contracts>
#include <cstdio>

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
}

static volatile int vz = 0;

int main() {
  int a[4] = {};
  int i = vz + 4;
  int before = reports;
  volatile int x = a[i];
  std::printf("bounds %d\n", reports - before);

  alignas(int) char buf[8] = {};
  int *pi = (int *)(buf + 1 + vz);
  before = reports;
  volatile int y = *pi;
  std::printf("align %d\n", reports - before);

  int big = INT_MAX + vz;
  before = reports;
  volatile int o = big + 1;
  std::printf("overflow %d\n", reports - before);

  int sh = 40 + vz;
  before = reports;
  volatile int h = 1 << sh;
  std::printf("shift %d\n", reports - before);
  (void)x;
  (void)y;
  (void)o;
  (void)h;
  return 0;
}

// CHECK: bounds 1
// CHECK-NEXT: align 1
// CHECK-NEXT: overflow 1
// CHECK-NEXT: shift 1
