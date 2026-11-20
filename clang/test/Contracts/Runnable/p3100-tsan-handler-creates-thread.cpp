// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe \
// RUN:   %libcxx_flags -ldl -o %t && %t

// Routed TSan calls the contract-violation handler after releasing its report,
// slot and thread-registry locks, so a handler may start a thread.  A watchdog
// thread, running before the report, turns a regression's hang into exit
// status 1.
//
// Mirror: gcc/testsuite/g++.dg/tsan/p3100-tsan-handler-creates-thread.C

#include <atomic>
#include <contracts>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

static std::atomic<bool> watching{false};
static void *watchdog(void *) {
  watching = true;
  sleep(10);
  syscall(SYS_exit_group, 1); // no sanitizer locks on the way out
  return nullptr;
}

static int joined = 0;
static void *helper(void *p) { return p; }
void handle_contract_violation(const std::contracts::contract_violation &) {
  pthread_t t;
  pthread_create(&t, nullptr, helper, nullptr);
  pthread_join(t, nullptr);
  joined = 1;
}

// A barrier TSan does not see, so the race on Global is detected.
static __typeof(pthread_barrier_wait) *invisible_barrier_wait;
static pthread_barrier_t barrier;
int Global;
static void *T1(void *) {
  invisible_barrier_wait(&barrier);
  usleep(1000);
  Global = 42;
  return nullptr;
}
static void *T2(void *) {
  Global = 43;
  invisible_barrier_wait(&barrier);
  return nullptr;
}

int main() {
  pthread_t w;
  pthread_create(&w, nullptr, watchdog, nullptr);
  while(!watching)
    ;
  void *h = dlopen("libpthread.so.0", RTLD_LAZY);
  invisible_barrier_wait =
      (__typeof(pthread_barrier_wait) *)dlsym(h, "pthread_barrier_wait");
  pthread_barrier_init(&barrier, nullptr, 2);
  pthread_t t[2];
  pthread_create(&t[0], nullptr, T1, nullptr);
  pthread_create(&t[1], nullptr, T2, nullptr);
  pthread_join(t[0], nullptr);
  pthread_join(t[1], nullptr);
  return joined ? 0 : 1;
}
