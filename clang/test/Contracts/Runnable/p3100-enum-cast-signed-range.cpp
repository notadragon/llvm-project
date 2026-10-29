// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.static.cast.enum.outside.range:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the enumeration-range check (ub:expr.static.cast.enum.outside.range)
// for enumerations with a negative enumerator, whose value range is signed:
// enum N {-1, 1} has the range [-2, 1] and enum M {-5, 3} the range [-8, 7].
// Values just outside each end are caught, the ends themselves are not, and
// so is an unsigned source whose value only wraps into the range (CLANG-731:
// 4294967295u was compared as -1), and sources narrower and wider than int.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-enum-cast-signed-range.C, for
// GCC-709)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
enum N { n1 = -1, n2 = 1 }; // values [-2, 1]
enum M { m1 = -5, m2 = 3 }; // values [-8, 7]
enum P { p0 = 0, p1 = 5 };  // values [0, 7]
template <typename E, typename T>
__attribute__((noinline)) int cast(T v) { return static_cast<E>(v); }
int main() {
  int r = 0;
  r += cast<N>(2);                 // caught -> 0
  r += cast<N>(-3);                // caught -> 0
  r += cast<N>(-2);                // -2
  r += cast<N>(1);                 // 1
  r += cast<M>(8);                 // caught -> 0
  r += cast<M>(-9);                // caught -> 0
  r += cast<M>(-8);                // -8
  r += cast<M>(7);                 // 7
  r += cast<P>(8);                 // caught -> 0
  r += cast<P>(-1);                // caught -> 0
  r += cast<P>(7);                 // 7
  r += cast<N>(4294967295u);       // caught (not -1) -> 0
  r += cast<N>((signed char)-3);   // caught -> 0
  r += cast<N>((signed char)-2);   // -2
  r += cast<M>(-9LL);              // caught -> 0
  r += cast<M>(0x100000007LL);     // caught (not 7) -> 0
  r += cast<M>((unsigned char)7);  // 7
  std::printf("RESULT r=%d viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=10 viols=10
