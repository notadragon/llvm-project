// CLANG-761.
// The handler's effects are not seen after a routed check the middle end
// inserts after optimizing, at -O1 and above.  Build with -O2 and one of
// -DLOCAL_BOUNDS -fsanitize=local-bounds, -DADDRESS -fsanitize=address,
// -DMEMORY -fsanitize=memory, -DTHREAD -fsanitize=thread, routed with
// -fsanitize-semantic=<name>:noexcept_observe; prints H, then 0 (expected 1).
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
