// RUN: %clangxx -std=c++17 %s -fsanitize=alignment %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-809 in this fork's bug-reports/).  Stock Clang: the
// arithmetic forms since 22 (#152575, upstream #177682), the element
// forms in every release measured (18.1.0 to trunk).
//
// -fsanitize=alignment does not check an access or a placement new
// through a pointer formed from a character array: `*(int *)(buf + o)`,
// `*(int *)&buf[o]`, `new (s + o) S()`, `new (&s[o]) S()`.  The check
// takes the alignment CodeGen derives for the address, which an explicit
// cast (and a placement argument) keeps from the array's declaration, the
// element's 1, so nothing is checked.  Through a pointer variable each is
// reported (the controls).  When every form is reported this XFAIL
// becomes an XPASS, the signal to close the bug.  The P3100 checks on
// this branch report every form (p3100-align-char-array-cast.cpp,
// p3100-placement-align-char-array.cpp; CLANG-810 fixed the access check).
// (GCC mirror: g++.dg/ubsan/align-char-array.C; gnu_gcc's
// -fsanitize=alignment reports the accesses and misses the placement
// forms, GCC-877.)

#include <cstdio>
#include <new>

static volatile int reports;
extern "C" void __ubsan_on_report() { reports = reports + 1; }

#define NI __attribute__((noinline))
struct alignas(4) S {};
alignas(8) char buf[16];
NI int load(int o) { return *(int *)(buf + o); }
NI void store(int o) { *(int *)(buf + o) = 1; }
NI int load_elem(int o) { return *(int *)&buf[o]; }
NI void pnew(int o) {
  alignas(S) char s[sizeof(S) + 1];
  new (s + o) S();
}
NI void pnew_elem(int o) {
  alignas(S) char s[sizeof(S) + 1];
  new (&s[o]) S();
}
NI int load_ptr(char *c, int o) { return *(int *)(c + o); }
NI void pnew_ptr(int o) {
  alignas(S) char s[sizeof(S) + 1];
  void *m = s + o;
  new (m) S();
}

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1;
  COUNT("load", load(one));
  COUNT("store", store(one));
  COUNT("load_elem", load_elem(one));
  COUNT("pnew", pnew(one));
  COUNT("pnew_elem", pnew_elem(one));
  COUNT("load_ptr", load_ptr(buf, one));
  COUNT("pnew_ptr", pnew_ptr(one));
  return 0;
}

// CHECK: load 1
// CHECK-NEXT: store 1
// CHECK-NEXT: load_elem 1
// CHECK-NEXT: pnew 1
// CHECK-NEXT: pnew_elem 1
// CHECK-NEXT: load_ptr 1
// CHECK-NEXT: pnew_ptr 1
