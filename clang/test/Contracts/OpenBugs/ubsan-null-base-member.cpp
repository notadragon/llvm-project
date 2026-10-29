// RUN: %clangxx -std=c++17 %s -fsanitize=null %libcxx_flags -o %t
// RUN: not %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-748 in this fork's bug-reports/).  Stock Clang; GCC
// reports every form.
//
// -fsanitize=null does not check a member of a base class used through a
// null pointer when the base is at a nonzero offset or virtual: the null
// check is applied to the pointer after its conversion to the base, which is
// not null for a nonzero offset, and for a virtual base the conversion loads
// the base's offset through the null pointer before any check (the program
// faults; the last form here).  A base at offset 0 is reported (the
// control).  When the other forms are reported too, this XFAIL becomes an
// XPASS, the signal to close the bug.  The P3100 check, which is fixed on
// this branch, is CLANG-747 (Runnable/p3100-null-base-member.cpp).
// (GCC mirror: g++.dg/ubsan/null-base-member.C, a passing test there:
// stock GCC -fsanitize=null reports every form.)

#include <cstdio>

struct B0 { int z; };
struct B {
  int b;
  __attribute__((noinline)) int f() { return 7; }
};
struct D1 : B { int x; };          // B at offset 0
struct D : B0, B { int x; };       // B at offset 4
struct VB : virtual B { int x; };

__attribute__((noinline)) int *first_addr(D1 *p) { return &p->b; }
__attribute__((noinline)) int *n_addr(D *p) { return &p->b; }
__attribute__((noinline)) int n_call(D *p) { return p->f(); }
__attribute__((noinline)) B &n_bind(D *p) { return *p; }
__attribute__((noinline)) int v_rd(VB *p) { return p->b; }

int main() {
  first_addr(nullptr);
  std::fprintf(stderr, "control done\n");
  n_addr(nullptr);
  n_call(nullptr);
  (void)&n_bind(nullptr);
  std::fprintf(stderr, "nonvirtual done\n");
  return v_rd(nullptr);
}

// CHECK: runtime error: {{.*}}null pointer of type
// CHECK: control done
// CHECK: runtime error: {{.*}}null pointer of type
// CHECK: runtime error: {{.*}}null pointer of type
// CHECK: runtime error: {{.*}}null pointer of type
// CHECK: nonvirtual done
// CHECK: runtime error: {{.*}}null pointer of type
