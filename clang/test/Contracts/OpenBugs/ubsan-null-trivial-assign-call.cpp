// RUN: %clangxx -std=c++17 %s -fsanitize=null %libcxx_flags -o %t
// RUN: not %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-763 in this fork's bug-reports/).  Stock Clang 18 to
// trunk; GCC reports it.
//
// -fsanitize=null does not check a trivial assignment operator called
// through a pointer, `sp->operator=(s)`: the IsArrow path of
// getLValueForThis in EmitCXXMemberOrOperatorMemberCallExpr ignores
// EmitCheckedForStore (the FIXME there), where `(*sp).operator=(s)` and
// `*sp = s` are checked as stores.  The store faults with no report.  When
// it is reported, this XFAIL becomes an XPASS, the signal to close the bug.
// The P3100 check, which is fixed on this branch, is CLANG-760
// (Runnable/p3100-null-trivial-member-call.cpp).
// (GCC mirror: g++.dg/ubsan/null-trivial-assign-call.C; stock GCC
// -fsanitize=null reports it.)

struct S { int m; };
__attribute__((noinline)) void assign(S *sp, S &s) { sp->operator=(s); }

int main() {
  S *volatile vs = nullptr;
  S s{};
  assign(vs, s);
  return 0;
}

// CHECK: runtime error: store to null pointer of type 'S'
