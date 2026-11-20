// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:assume \
// RUN:   -emit-llvm -o - %s | FileCheck %s

// P3100 (UBSan routing, assume): a member access and a member function call
// through a glvalue whose type is not similar to the object's
// (ub:expr.ref.member.not.similar) would be checked by -fsanitize=vptr
// ("member access within", "member call on"); resolving the check to
// "assume" turns it off -- no vptr runtime call is emitted.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-vptr-member-call-assume-codegen.C)

struct A {
  virtual ~A() {}
  int i = 1;
};
struct B {
  virtual ~B() {}
  int j = 2;
  int f();
};

int call(A &a) { return reinterpret_cast<B &>(a).f(); }
int dot(A &a) { return reinterpret_cast<B &>(a).j; }

// CHECK-NOT: __ubsan_handle_dynamic_type_cache_miss
