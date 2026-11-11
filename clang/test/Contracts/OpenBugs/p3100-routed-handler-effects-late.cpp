// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 -O2 -w \
// RUN:   -DLOCAL_BOUNDS -fsanitize=local-bounds \
// RUN:   -fsanitize-semantic=local-bounds:noexcept_observe \
// RUN:   %libcxx_flags -o %t.lb
// RUN: %t.lb | FileCheck %s --check-prefix=LB
// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 -O2 -w \
// RUN:   -DADDRESS -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t.asan
// RUN: %t.asan 2>&1 | FileCheck %s --check-prefix=ASAN
// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 -O2 -w \
// RUN:   -DMEMORY -fsanitize=memory -fsanitize-semantic=memory:noexcept_observe \
// RUN:   %libcxx_flags -o %t.msan
// RUN: %t.msan 2>&1 | FileCheck %s --check-prefix=MSAN
// RUN: %clangxx -std=c++26 %s -fcontracts-p3100 -fcontracts-p4298 -O2 -w \
// RUN:   -DTHREAD -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe \
// RUN:   %libcxx_flags -ldl -o %t.tsan
// RUN: %t.tsan 2>&1 | FileCheck %s --check-prefix=TSAN
// XFAIL: *

// OPEN BUG (CLANG-761 in this fork's open-issues/).
//
// The effects of the contract-violation handler reached from a sanitizer
// check that the middle end inserts after optimizing are not seen by the
// code after the check at -O1 and above: local-bounds (the BoundsChecking
// pass, at the end of the function simplification pipeline), address,
// memory and thread (their instrumentation passes, at the end of the
// optimization pipeline).  The passes that ran before saw no call at the
// access, so they kept the value of `reports`, a static only the handler
// changes, loaded before it.  Each check reports once (the handler prints
// "H") and the program continues, but the count printed is 0.  Expected:
// 1 each, as at -O0 and as for the checks the front end emits
// (Runnable/p3100-routed-handler-effects.cpp,
// Runnable/p3100-handler-effects-inlined.cpp).  When one is fixed, its RUN
// line passes; the file XPASSes when all four do.
// (GCC: the address and thread checks lose the effects the same way,
// gnu_gcc GCC-806, watch tests
// g++.dg/{asan,tsan}/open-bug-routed-handler-effects-late.C; GCC has no
// local-bounds or memory.  Both are deferred by the user as a documented
// limitation, DECISIONS.md E90 (b).)

#include <contracts>
#include <cstdio>
#include <cstdlib>
#if defined(THREAD)
#include <pthread.h>
#include <unistd.h>
#endif

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
  std::puts("H");
}

static volatile int vz = 0;
#if defined(THREAD)
int shared;
static void *writer(void *) {
  shared = 1;
  return nullptr;
}
#endif

int main() {
#if defined(LOCAL_BOUNDS)
  int a[4] = {1, 2, 3, 4};
  int i = vz + 4;
  int before = reports;
  volatile int x = a[i];
  std::printf("local-bounds %d\n", reports - before);
  (void)x;
#elif defined(ADDRESS)
  int *h = (int *)std::malloc(4 * sizeof(int));
  for (int k = 0; k < 4; ++k)
    h[k] = k + vz;
  int i = vz + 4;
  int before = reports;
  volatile int x = h[i];
  std::printf("address %d\n", reports - before);
  (void)x;
  std::free(h);
#elif defined(MEMORY)
  int *u = (int *)std::malloc(sizeof(int));
  int *volatile vu = u;
  u = vu;
  int before = reports;
  if (*u + vz)
    vz = 1;
  std::printf("memory %d\n", reports - before);
  std::free(u);
#elif defined(THREAD)
  pthread_t t;
  pthread_create(&t, nullptr, writer, nullptr);
  usleep(100000);
  int before = reports;
  shared = 2;
  std::printf("thread %d\n", reports - before);
  pthread_join(t, nullptr);
#endif
  return 0;
}

// LB: H
// LB-NEXT: local-bounds 1
// ASAN: H
// ASAN: address 1
// MSAN: H
// MSAN: memory 1
// TSAN: H
// TSAN: thread 1
