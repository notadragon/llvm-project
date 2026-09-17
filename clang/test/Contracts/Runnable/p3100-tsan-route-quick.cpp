// P3100: -fsanitize-semantic=thread:quick_enforce terminates silently on the
// first detected data race WITHOUT entering the handler and WITHOUT printing
// anything (the routed sanitizer emits nothing).  Accepted without
// -fcontracts-p4298.  Nonzero exit (wrapped with `not`); no output.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:quick_enforce \
// RUN:   %libcxx_flags -ldl -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>

// Must NOT run under quick_enforce.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d\n", (int)v.kind());
  std::fflush(stdout);
}

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
  std::printf("NOTREACHED\n");
  std::fflush(stdout);
  return 0;
}

// quick_enforce = silent terminate: no handler, no output.
// CHECK-NOT: handler
// CHECK-NOT: NOTREACHED
// CHECK-NOT: ThreadSanitizer
