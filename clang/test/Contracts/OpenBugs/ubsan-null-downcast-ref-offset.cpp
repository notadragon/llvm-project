// RUN: %clangxx -std=c++17 %s -fsanitize=null -fsanitize-recover=null %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s
// XFAIL: *

// OPEN BUG (CLANG-803 in this fork's bug-reports/).  Stock Clang 18 to
// trunk.
//
// -fsanitize=null does not check a downcast of `*p` to a reference, `p`
// null, when the base is at a nonzero offset in the derived class: the
// downcast's null check ("downcast of null pointer") and the use's (a
// member access, a reference binding, a member call) test the derived
// object's address, `p` minus the base's offset, which is not null.  A
// base at offset zero is reported (the control).  No form here reads
// through the derived object, so none faults.  When the other forms are
// reported too, this XFAIL becomes an XPASS, the signal to close the bug.
// The P3100 check, which is fixed on this branch, is CLANG-802
// (Runnable/p3100-null-downcast-ref-offset.cpp).
// (GCC mirror: g++.dg/ubsan/null-downcast-ref-offset.C.)

#include <cstdio>

struct V {
  virtual void h() {}
  int v;
};
struct X {
  virtual void x() {}
  long pad;
};
struct D0 : V { int d; };                // V at offset 0
struct D2 : X, V {                       // V at offset 16
  int d;
  __attribute__((noinline)) void dn() {}
};
__attribute__((noinline)) D0 &bind0(V *p) { return static_cast<D0 &>(*p); }
__attribute__((noinline)) D2 &bind(V *p) { return static_cast<D2 &>(*p); }
__attribute__((noinline)) int *addr(V *p) {
  return &static_cast<D2 &>(*p).d;
}
__attribute__((noinline)) void call(V *p) { static_cast<D2 &>(*p).dn(); }

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  V *volatile p = nullptr;
  std::printf("bind0\n");
  bind0(p);
  std::printf("bind\n");
  bind(p);
  std::printf("addr\n");
  addr(p);
  std::printf("call\n");
  call(p);
  std::printf("done\n");
  return 0;
}

// CHECK: bind0
// CHECK-NEXT: runtime error: downcast of null pointer of type 'D0'
// CHECK: bind
// CHECK-NEXT: runtime error: downcast of null pointer of type 'D2'
// CHECK: addr
// CHECK-NEXT: runtime error: downcast of null pointer of type 'D2'
// CHECK: call
// CHECK-NEXT: runtime error: downcast of null pointer of type 'D2'
// CHECK: done
