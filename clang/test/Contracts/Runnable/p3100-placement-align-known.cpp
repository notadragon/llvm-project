// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a placement new into a local array at an offset that misaligns the
// object (ub:basic.align.object.alignment) under "observe": the pointer is
// checked against the alignment of the type created, not against the
// alignment the array is known to have at that offset -- an over-aligned
// empty class (.nostore), and an initialization that stores (.store, once)
// are caught; an aligned offset is not.  (Clang used to check against the
// known alignment of the address, 1 here, and missed all of them: CLANG-709.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-align-known.C, where
// the .store check is made in the middle end, under its noexcept form)

#include <contracts>
#include <cstdio>
#include <new>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
  std::printf("VIOL comment=[%s]\n", v.comment());
}
struct alignas(4) S {};
struct T {
  int a;
};
int main() {
  alignas(8) char s[16];
  new (s + 4) S();
  new (s + 4) int(5);
  std::printf("viols=%d\n", viols);
  new (s + 1) S();
  new (s + 1) int(5);
  new (s + 1) T{5};
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: viols=0
// CHECK-NEXT: VIOL comment=[misaligned pointer passed to a non-allocating placement new]
// CHECK-NEXT: VIOL comment=[misaligned pointer access]
// CHECK-NEXT: VIOL comment=[misaligned pointer access]
// CHECK-NEXT: viols=3
