// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A contract assertion met while deciding whether a const local is
// constant-initialized is evaluated, and a false or not-constant predicate
// recorded.  When the rest of the initializer is not constant either,
// nothing recorded is reported: the local is initialized at run time and the
// predicate is evaluated there.  (GCC-618 on the GCC side, fixed there.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/precondition-const-local-init.C

// The precondition reads the caller's parameter, which is not a constant.
constexpr int get_one(const int &value) pre(value != 0) {
  return value / value;
}

constexpr int wrapper(int divisor) {
  const int step = get_one(divisor);
  return step;
}

static_assert(wrapper(8) == 1);

// The precondition is constant and false, but the body divides by zero, so
// the initializer is not constant either way and the violation belongs to
// run time.
constexpr int get_one_b(const int &value) pre(value != 0) {
  return value / value;
}

int runtime_only() {
  const int step = get_one_b(0);
  return step;
}
