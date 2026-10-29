// P3100 RF: on the routed path the sanitizer emits NOTHING; the handler owns all
// output and retrieves the sanitizer's description on demand via
// contract_violation::report() (requires -fcontracts-p4301).  v1: libtsan
// returns a concise description; full multi-line capture is a documented
// follow-up.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=thread \
// RUN:   -fsanitize-semantic=thread:noexcept_observe %libcxx_flags -ldl -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *r = v.report();
  std::printf("report=%s\n", r ? r : "(null)");
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
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The report() text (concise v1 description) is the first output; nothing leaks
// before it.
// CHECK: report=ThreadSanitizer: data race
// CHECK: survived
