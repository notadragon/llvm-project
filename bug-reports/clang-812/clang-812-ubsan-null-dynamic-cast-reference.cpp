// CLANG-812: -fsanitize=null does not check the operand of a dynamic_cast
// to a reference type, `dynamic_cast<W &>(*vp)` with `vp` null: the cast
// calls __dynamic_cast with the null pointer with no report (which faults
// with libc++abi, or throws std::bad_cast with libsupc++).  clang++
// -std=c++17 -fsanitize=null, run: prints the number of UBSan reports of
// each form (a fault is caught and resumed, std::bad_cast caught); 1 is
// expected for every form.  Clang 18.1.0..trunk: star=0 cstar=0 sub=0; the
// member access control `hp->m` is reported.  GCC reports each.
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
static volatile int reports;
extern "C" void __ubsan_on_report() { reports = reports + 1; }
static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }
struct V { virtual void h() {} int v; };
struct W : V {};
struct H { int x; V m; };
#define NI __attribute__((noinline))
NI void star(V *vp) { try { (void)dynamic_cast<W &>(*vp); } catch (...) {} }
NI void cstar(V *vp) {
  try { (void)dynamic_cast<const W &>(*vp); } catch (...) {}
}
NI void sub(V *vp) { try { (void)dynamic_cast<W &>(vp[0]); } catch (...) {} }
// Control: a member access through the null pointer.
NI void member(H *hp) {
  try { (void)dynamic_cast<W &>(hp->m); } catch (...) {}
}
#define COUNT(name, ...)                                                      \
  do {                                                                        \
    int before = reports;                                                     \
    if (!sigsetjmp(env, 1)) {                                                 \
      __VA_ARGS__;                                                            \
    }                                                                         \
    std::printf(name "=%d ", reports - before);                               \
  } while (0)
int main() {
  dup2(open("/dev/null", O_WRONLY), 2);  // The counts are the output.
  std::signal(SIGSEGV, on_segv);
  V *volatile vp = nullptr;
  H *volatile hp = nullptr;
  COUNT("star", star(vp));
  COUNT("cstar", cstar(vp));
  COUNT("sub", sub(vp));
  COUNT("member", member(hp));
  std::printf("\n");
  return 0;
}
