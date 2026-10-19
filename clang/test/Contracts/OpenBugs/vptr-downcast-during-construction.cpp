// RUN: %clangxx -std=c++17 %s -fsanitize=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-688 in this fork's bug-reports/, llvm#59060; deferred by
// the user 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-717).
//
// -fsanitize=vptr reports a static_cast downcast to F made in E's
// constructor while an F is being constructed.  The downcast is defined
// ([expr.static.cast]/2: the B is a base subobject of an F), but the vptr
// check compares against the dynamic type during construction, E.  The
// CHECK lines expect no report before "done", so when the false report goes
// away this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/ubsan/open-bug-vptr-downcast-during-construction.C)

#include <cstdio>

struct B { virtual ~B() {} };
struct E : B { E(); };
struct F : E { int x = 0; };
E::E() { (void)static_cast<F *>(static_cast<B *>(this)); }

int main() {
  F f;
  std::fprintf(stderr, "done\n");
  return 0;
}

// CHECK-NOT: runtime error
// CHECK: done
