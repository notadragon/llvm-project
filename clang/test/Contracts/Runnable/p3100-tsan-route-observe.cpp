// P3100: under -fcontracts-p3100 a ThreadSanitizer-detected data race is routed
// to the contract-violation handler.  With -fcontracts-p4298 and
// -fsanitize-semantic=thread:noexcept_observe the thread check resolves to
// noexcept_observe: the handler runs (kind=7, semantic=6) and the program
// CONTINUES, exiting normally (the routed observe report is not counted, so TSan
// does not force a nonzero exit).

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe \
// RUN:   %libcxx_flags -ldl -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

// TSAN-invisible barrier: resolve pthread_barrier_wait via dlsym so TSan does
// NOT model it as synchronization -- it provides scheduling overlap without
// establishing happens-before, so the race on Global is genuinely detected.
static __typeof(pthread_barrier_wait) *invisible_barrier_wait;
static pthread_barrier_t barrier;
static void barrier_setup() {
  void *h = dlopen("libpthread.so.0", RTLD_LAZY);
  invisible_barrier_wait =
      (__typeof(pthread_barrier_wait) *)dlsym(h, "pthread_barrier_wait");
  pthread_barrier_init(&barrier, nullptr, 2);
}

int Global;

static void *Thread1(void *) {
  invisible_barrier_wait(&barrier);
  usleep(1000);
  Global = 42;
  return nullptr;
}

static void *Thread2(void *) {
  Global = 43;
  invisible_barrier_wait(&barrier);
  return nullptr;
}

int main() {
  barrier_setup();
  pthread_t t[2];
  pthread_create(&t[0], nullptr, Thread1, nullptr);
  pthread_create(&t[1], nullptr, Thread2, nullptr);
  pthread_join(t[0], nullptr);
  pthread_join(t[1], nullptr);
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The handler runs (kind 7, noexcept_observe = 6); nothing leaks before it.
// CHECK: handler kind=7 semantic=6
// CHECK: survived
