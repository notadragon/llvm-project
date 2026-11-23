// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe \
// RUN:   -fcontracts-p4298 -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe \
// RUN:   %libcxx_flags -o %t.san
// RUN: %t.san | FileCheck %s

// P3100: a misaligned placement new that stores nothing
// (ub:basic.align.object.alignment.nostore) is reported when its pointer
// is arithmetic on a character array, `new (s + o) S()`, or the address of
// an element, `new (&s[o]) S`, as through a pointer variable -- the empty
// class and default-initialization alike -- once each, at -O0 and -O2, and
// once with -fsanitize=alignment routed too.  The native check tests the
// allocated type's alignment.  Stock -fsanitize=alignment misses the
// arithmetic and element forms (CLANG-809: the alignment derived from the
// array's declaration is kept for the placement argument; since 22 for
// `s + o`, always for `&s[o]`; deferred, DECISIONS.md E127 (a)).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-placement-align-char-array*.C (gnu_gcc's -fsanitize=alignment
// checks neither form, GCC-877).

#include <contracts>
#include <cstdio>
#include <new>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
struct alignas(4) S {};
struct T { int a, b; };
NI void arith(int o) {
  alignas(S) char s[sizeof(S) + 1];
  new (s + o) S();
}
NI void var(int o) {
  alignas(S) char s[sizeof(S) + 1];
  void *m = s + o;
  new (m) S();
}
NI void arith_dflt(int o) {
  alignas(T) char s[sizeof(T) + 1];
  new (s + o) T;
}
NI void var_dflt(int o) {
  alignas(T) char s[sizeof(T) + 1];
  void *m = s + o;
  new (m) T;
}
NI void elem(int o) {
  alignas(S) char s[sizeof(S) + 1];
  new (&s[o]) S;
}

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1, zero = 0;
  COUNT("arith", arith(one));
  COUNT("var", var(one));
  COUNT("arith_dflt", arith_dflt(one));
  COUNT("var_dflt", var_dflt(one));
  COUNT("elem", elem(one));
  COUNT("aligned", arith(zero); var(zero); arith_dflt(zero); var_dflt(zero);
        elem(zero));
  return 0;
}

// CHECK: arith 1
// CHECK-NEXT: var 1
// CHECK-NEXT: arith_dflt 1
// CHECK-NEXT: var_dflt 1
// CHECK-NEXT: elem 1
// CHECK-NEXT: aligned 0
