// RUN: %clangxx -std=c++17 %s -fsanitize=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-734 in this fork's bug-reports/; deferred by the user
// 2026-10-13, DECISIONS.md E76).  Stock Clang and GCC alike (gnu_gcc
// GCC-773).
//
// -fsanitize=vptr does not check a member of a polymorphic base class reached
// through a glvalue of the wrong derived type ([expr.ref],
// ub:expr.ref.member.not.similar): the member access is instrumented against
// the base subobject, whose dynamic type is right.  A member of the derived
// class itself is reported (the control).  When the inherited member is
// reported too, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/ubsan/open-bug-vptr-base-member.C)

#include <cstdio>

struct Base { virtual ~Base() {} int x = 1; };
struct A : Base { int a = 2; };
struct B : Base { int b = 3; };

__attribute__((noinline)) int own(A &r) { return reinterpret_cast<B &>(r).b; }
__attribute__((noinline)) int inherited(A &r) {
  return reinterpret_cast<B &>(r).x;
}

int main() {
  A a;
  own(a);
  std::fprintf(stderr, "control done\n");
  inherited(a);
  return 0;
}

// CHECK: vptr-base-member.cpp:23:{{[0-9]+}}: runtime error: member access within address 0x{{[0-9a-f]+}} which does not point to an object of type 'B'
// CHECK: control done
// CHECK-NEXT: vptr-base-member.cpp:25:{{[0-9]+}}: runtime error: member access within address 0x{{[0-9a-f]+}} which does not point to an object of type 'B'
