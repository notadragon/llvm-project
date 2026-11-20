// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t

// Routed ASan calls the contract-violation handler after releasing the thread
// registry lock, so a handler may start a thread.  A watchdog thread, running
// before the report, turns a regression's hang into exit status 1.  The
// handler also calls report(), whose render takes the registry itself.
//
// Mirror: gcc/testsuite/g++.dg/asan/p3100-asan-handler-creates-thread.C

#include <atomic>
#include <contracts>
#include <cstdlib>
#include <cstring>
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
void handle_contract_violation(const std::contracts::contract_violation &v) {
  pthread_t t;
  pthread_create(&t, nullptr, helper, nullptr);
  pthread_join(t, nullptr);
  if (!std::strstr(v.report(), "heap-buffer-overflow"))
    std::abort();
  joined = 1;
}

int main() {
  pthread_t w;
  pthread_create(&w, nullptr, watchdog, nullptr);
  while(!watching)
    ;
  char *volatile p = static_cast<char *>(std::malloc(8));
  p[8] = 1; // heap-buffer-overflow
  std::free(p);
  return joined ? 0 : 1;
}
