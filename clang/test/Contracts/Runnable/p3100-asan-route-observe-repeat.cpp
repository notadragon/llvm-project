// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t \
// RUN:   && %t 2>&1 \
// RUN:   | FileCheck %s

// P3100: a routed noexcept_observe ASan check must report every DISTINCT
// violating site, however many there are.
//
// SuppressErrorReport dedups by program counter using a fixed 25-entry pool
// (kAsanBuggyPcPoolSize).  Once that filled, the routed path returned
// "suppress" for everything, so every subsequent routed violation anywhere
// in the process was dropped -- including at sites never reported once.
// With 30 distinct sites only 25 handler calls arrived.
//
// Per-site dedup of REPEATS is deliberate on the routed path and is
// asserted by p3100-asan-route-observe-multi.cpp; this test does not
// disturb it.  What it pins down is that running out of dedup capacity
// must not turn into "stop checking".
//
// GCC mirror: g++.dg/asan/p3100-asan-route-observe-repeat.C
// (gnu_gcc 75813de1471), where the runtime source is identical.

#include <contracts>
#include <cstdio>
#include <cstdlib>

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
}

volatile int sink;

// 30 distinct out-of-bounds sites, comfortably past the 25-entry pool.
// noipa rather than noinline: the bodies are identical, so with
// interprocedural optimisation enabled they could fold to a single
// function and hence a single PC, and the test would quietly stop
// testing anything.
#define SITE(N)                                                                \
  __attribute__((noipa)) static int site_##N(int *p, int i) { return p[i]; }
#define SITES10(B)                                                             \
  SITE(B##0) SITE(B##1) SITE(B##2) SITE(B##3) SITE(B##4)                       \
  SITE(B##5) SITE(B##6) SITE(B##7) SITE(B##8) SITE(B##9)
SITES10(0)
SITES10(1)
SITES10(2)

#define CALL10(B, P, I)                                                        \
  sink = site_##B##0(P, I); sink = site_##B##1(P, I);                          \
  sink = site_##B##2(P, I); sink = site_##B##3(P, I);                          \
  sink = site_##B##4(P, I); sink = site_##B##5(P, I);                          \
  sink = site_##B##6(P, I); sink = site_##B##7(P, I);                          \
  sink = site_##B##8(P, I); sink = site_##B##9(P, I)

int main(int argc, char **) {
  int *p = (int *)std::malloc(3 * sizeof(int));
  if (!p)
    return 1;
  int i = argc + 4; // >= 5, heap OOB, not constant-foldable

  CALL10(0, p, i);
  CALL10(1, p, i);
  CALL10(2, p, i);

  std::free(p);
  std::printf("done calls=%d\n", calls);
  std::fflush(stdout);
  return 0;
}

// Every distinct site delivered, none lost to pool exhaustion.
// CHECK: done calls=30
