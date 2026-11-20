// RUN: %clangxx -std=c++17 %s -fsanitize=null %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-812 in this fork's bug-reports/).  Stock Clang 18 to
// trunk.
//
// -fsanitize=null does not check the operand of a dynamic_cast to a
// reference type, `dynamic_cast<W &>(*vp)` with `vp` null: the operand's
// check (TCK_DynamicOperation) allows a null pointer, right for the
// pointer form only, and __dynamic_cast is then called with the null
// pointer (a fault, caught here).  The member access `hp->m` as the
// operand is reported (the control).  When the other forms are reported
// too, this XFAIL becomes an XPASS, the signal to close the bug.  The
// P3100 check reports every form (p3100-null-dynamic-cast-reference.cpp);
// with its leaf not configured and the sanitizer routed, the forms report
// 0 because of this bug (p3100-null-dynamic-cast-reference-sanitize-null
// .cpp).  (gnu_gcc reports each; no GCC mirror needed beyond its
// ubsan/p3100-null-dynamic-cast-reference-sanitize-null*.C.)

#include <csetjmp>
#include <csignal>
#include <cstdio>

static volatile int reports;
extern "C" void __ubsan_on_report() { reports = reports + 1; }
static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct V { virtual void h() {} int v; };
struct W : V {};
struct H { int x; V m; };
#define NI __attribute__((noinline))
NI void star(V *vp) {
  try { (void)dynamic_cast<W &>(*vp); } catch (...) {}
}
NI void cstar(V *vp) {
  try { (void)dynamic_cast<const W &>(*vp); } catch (...) {}
}
NI void sub(V *vp) {
  try { (void)dynamic_cast<W &>(vp[0]); } catch (...) {}
}
NI void member(H *hp) {
  try { (void)dynamic_cast<W &>(hp->m); } catch (...) {}
}

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  V *volatile vp = nullptr;
  H *volatile hp = nullptr;
  COUNT("star", star(vp));
  COUNT("cstar", cstar(vp));
  COUNT("sub", sub(vp));
  COUNT("member", member(hp));
  return 0;
}

// CHECK: star 1
// CHECK-NEXT: cstar 1
// CHECK-NEXT: sub 1
// CHECK-NEXT: member 1
