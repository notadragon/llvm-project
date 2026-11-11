// CLANG-809: -fsanitize=alignment does not check an access or a placement
// new through a pointer formed from a character array: `*(int *)(buf + o)`
// (load or store) and `new (s + o) S()` since Clang 22 (#152575), and
// `*(int *)&buf[o]` and `new (&s[o]) S()` in every release measured.
// clang++ -std=c++17 -fsanitize=alignment, run: prints the number of UBSan
// reports of each form, with o = 1 (misaligned); 1 is expected for every
// form.  Clang 18.1.0..21.1.0: load=1 store=1 load_elem=0 pnew=1
// pnew_elem=0; 22.1.6 and trunk: all 0.  The pointer-variable controls are
// reported by every release; GCC reports the loads and stores.
#include <cstdio>
#include <fcntl.h>
#include <new>
#include <unistd.h>
static volatile int reports;
extern "C" void __ubsan_on_report() { reports = reports + 1; }
#define NI __attribute__((noinline))
struct alignas(4) S {};
alignas(8) char buf[16];
NI int load(int o) { return *(int *)(buf + o); }
NI void store(int o) { *(int *)(buf + o) = 1; }
NI int load_elem(int o) { return *(int *)&buf[o]; }
NI void pnew(int o) { alignas(S) char s[sizeof(S) + 1]; new (s + o) S(); }
NI void pnew_elem(int o) { alignas(S) char s[sizeof(S) + 1]; new (&s[o]) S(); }
// Controls: the same pointers through a variable.
NI int load_ptr(char *c, int o) { return *(int *)(c + o); }
NI void pnew_ptr(int o) {
  alignas(S) char s[sizeof(S) + 1];
  void *m = s + o;
  new (m) S();
}
#define COUNT(name, ...)                                                      \
  do {                                                                        \
    int before = reports;                                                     \
    __VA_ARGS__;                                                              \
    std::printf(name "=%d ", reports - before);                               \
  } while (0)
int main() {
  dup2(open("/dev/null", O_WRONLY), 2);  // The counts are the output.
  int volatile one = 1;
  COUNT("load", load(one));
  COUNT("store", store(one));
  COUNT("load_elem", load_elem(one));
  COUNT("pnew", pnew(one));
  COUNT("pnew_elem", pnew_elem(one));
  COUNT("load_ptr", load_ptr(buf, one));
  COUNT("pnew_ptr", pnew_ptr(one));
  std::printf("\n");
  return 0;
}
