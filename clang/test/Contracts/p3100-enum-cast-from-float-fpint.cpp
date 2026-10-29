// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-enum-cast-from-float-fpint.json %libcxx_flags -o %t && %t

// P3100: the fpint half of a floating-point -> enumeration static_cast is
// checked against the underlying type, not the enumeration's range: 7.0
// fits the underlying type, so with ub:conv.fpint observed and the enum
// group ignored nothing is reported; 1e30 does not fit and is reported, as
// does 300.0 for an enumeration whose fixed underlying type is unsigned char.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-enum-cast-from-float-fpint.C

#include <contracts>
#include <cstring>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (std::strstr(v.comment(), "floating-point"))
    ++reported;
}

enum E { a, b, c }; // range of values 0..3
enum class F : unsigned char { x };
volatile double d = 7.0;
volatile double big = 1e30;
volatile double over = 300.0;

int main() {
  E e1 = static_cast<E>(d); // in the underlying type: not reported
  if (reported != 0)
    __builtin_abort();
  E e2 = static_cast<E>(big); // not in the underlying type: reported
  if (reported != 1)
    __builtin_abort();
  F f = static_cast<F>(over); // not in unsigned char: reported
  if (reported != 2)
    __builtin_abort();
  (void)e1;
  (void)e2;
  (void)f;
}

// REQUIRES: contracts-libcxx, native
