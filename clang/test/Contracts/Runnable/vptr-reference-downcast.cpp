// RUN: %clangxx -std=c++17 %s -fsanitize=vptr %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

// -fsanitize=vptr checks the reference form of a static_cast downcast, as it
// does the pointer form: an lvalue or xvalue of type B cast to D& or D&& when
// the object is not a D, including a base at a nonzero offset.  Stock Clang
// does; GCC did not (PR116102, gnu_gcc GCC-734, fixed there).  No contracts
// involved.
// (GCC mirror: g++.dg/ubsan/vptr-reference-downcast.C)

#include <cstdio>

struct S { virtual ~S() {} int s = 1; };
struct T : S { int t = 2; };
struct U : S { int u = 3; };
struct V { virtual ~V() {} long v = 4; };
struct W : V, S { int w = 5; }; // S at a nonzero offset
struct X : V, S { int x = 6; }; // the same layout, not a W

__attribute__((noinline)) int lref(S &r) { return static_cast<T &>(r).s; }
__attribute__((noinline)) int rref(S &r) {
  return static_cast<T &&>(static_cast<S &&>(r)).s;
}
__attribute__((noinline)) int cref(const S &r) {
  return static_cast<const T &>(r).s;
}
__attribute__((noinline)) int offset(S &r) { return static_cast<W &>(r).s; }

int main() {
  T t; U u; S s; W w; X x;
  // These do not report: the object is of the target type.
  lref(t);
  rref(t);
  cref(t);
  offset(w);
  std::fprintf(stderr, "clean\n");
  // These do.
  lref(u);
  rref(s);
  cref(u);
  offset(x);
  return 0;
}

// CHECK-NOT: runtime error
// CHECK: clean
// CHECK-NEXT: vptr-reference-downcast.cpp:20:{{[0-9]+}}: runtime error: downcast of address 0x{{[0-9a-f]+}} which does not point to an object of type 'T'
// CHECK-NEXT: 0x{{[0-9a-f]+}}: note: object is of type 'U'
// CHECK: vptr-reference-downcast.cpp:22:{{[0-9]+}}: runtime error: downcast of address 0x{{[0-9a-f]+}} which does not point to an object of type 'T'
// CHECK-NEXT: 0x{{[0-9a-f]+}}: note: object is of type 'S'
// CHECK: vptr-reference-downcast.cpp:25:{{[0-9]+}}: runtime error: downcast of address 0x{{[0-9a-f]+}} which does not point to an object of type 'const T'
// CHECK-NEXT: 0x{{[0-9a-f]+}}: note: object is of type 'U'
// CHECK: vptr-reference-downcast.cpp:27:{{[0-9]+}}: runtime error: downcast of address 0x{{[0-9a-f]+}} which does not point to an object of type 'W'
// CHECK-NEXT: 0x{{[0-9a-f]+}}: note: object is of type 'X'
