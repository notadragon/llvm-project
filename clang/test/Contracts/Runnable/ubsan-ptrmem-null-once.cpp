// RUN: %clangxx -std=c++17 %s -fsanitize=null -fsanitize-recover=null \
// RUN:   %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++17 %s -O2 -fsanitize=null -fsanitize-recover=null \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 2>&1 | FileCheck %s

// With -fsanitize=null alone (no contract options), a pointer to data
// member applied through a null pointer, `sp->*pm` with pm the member at
// offset zero, is reported once: the load through the member's address,
// which is null.  Clang makes no null check of the object pointer of
// `->*` without -fcontracts-p3100.
// Mirror: gcc/testsuite/g++.dg/ubsan/open-bug-ptrmem-null-twice.C, the
// watch test of gnu_gcc GCC-853 (reported twice there: the branch's check
// of the object pointer of `->*` and the load's).

#include <csetjmp>
#include <csignal>
#include <cstdio>

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct S { int a; int b; };

__attribute__((noinline)) int pmd(S *sp, int S::*pm) { return sp->*pm; }

int main() {
  std::signal(SIGSEGV, on_segv);
  S *volatile sp = nullptr;
  if (!sigsetjmp(env, 1))
    pmd(sp, &S::a);
  std::fprintf(stderr, "done\n");
  return 0;
}

// CHECK: runtime error: load of null pointer
// CHECK-NOT: runtime error
// CHECK: done
