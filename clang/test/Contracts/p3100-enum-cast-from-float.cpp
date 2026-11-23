// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-enum-cast-from-float.json %libcxx_flags -o %t && %t

// P3100: a static_cast from floating point to an enumeration without a
// fixed underlying type converts to the underlying type and then to the
// enumeration ([expr.static.cast]), so an out-of-range result is
// ub:expr.static.cast.enum.outside.range exactly as for an integer source
// (7.0 fits the underlying type, so it is not ub:conv.fpint; see
// p3100-enum-cast-from-float-fpint.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-enum-cast-from-float.C

#include <contracts>
#include <cstring>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (std::strstr(v.comment(), "enumeration"))
    ++reported;
}

enum E { a, b, c }; // range of values 0..3
volatile double d = 7.0;
volatile int i = 7;

int main() {
  E e1 = static_cast<E>(i); // control: reported
  if (reported != 1)
    __builtin_abort();
  E e2 = static_cast<E>(d); // must be reported the same way
  if (reported != 2)
    __builtin_abort();
  (void)e1;
  (void)e2;
}

// REQUIRES: contracts-libcxx, native
