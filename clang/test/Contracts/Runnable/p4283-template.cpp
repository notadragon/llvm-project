// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P4283: requires-clauses in various template contexts, run: members of a
// class template (a precondition and a postcondition with a result name,
// constrained differently), a function template mixing an unconstrained
// assertion with constrained ones, and a member function template.  Each
// kept assertion is checked (two are violated); each discarded one is not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-template.C

#include <concepts>
#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violation_count;
}

// Class template with requires on member function contracts.
template <typename T> struct Container {
  T value;

  void set(T v) pre requires (std::totally_ordered<T>) (v > value) {
    value = v;
  }

  T get() const post requires (std::integral<T>) (r : r >= 0) {
    return value;
  }
};

// Function template with multiple contracts, only some constrained.
template <typename T>
T clamp_positive(const T x)
  pre(true)
  pre requires (std::signed_integral<T>) (x > -1000)
  post requires (std::integral<T>) (r : r >= 0) {
  if (x < T{})
    return T{};
  return x;
}

// Member function template.
struct Validator {
  template <typename T> void check(T x) pre requires (std::integral<T>) (x != 0) {}
};

int main() {
  // Class template: int -- contracts active (totally_ordered and integral).
  Container<int> ci{0};
  violation_count = 0;
  ci.set(10); // pre satisfied (10 > 0)
  if (violation_count != 0) __builtin_abort();
  ci.get(); // post satisfied (10 >= 0)
  if (violation_count != 0) __builtin_abort();

  // Class template: double -- totally_ordered satisfied but not integral.
  Container<double> cd{0.0};
  violation_count = 0;
  cd.set(1.5); // pre active, satisfied (1.5 > 0.0)
  if (violation_count != 0) __builtin_abort();
  cd.value = -2.0;
  cd.get(); // post discarded (not integral<double>), though r < 0
  if (violation_count != 0) __builtin_abort();

  // Multiple contracts, mixed constrained/unconstrained: int.
  violation_count = 0;
  clamp_positive(5); // all satisfied
  if (violation_count != 0) __builtin_abort();

  // unsigned int: signed_integral unsatisfied, discarded.
  violation_count = 0;
  clamp_positive(5u); // pre(true) active, pre requires discarded, post active
  if (violation_count != 0) __builtin_abort();

  // double: signed_integral and integral unsatisfied.
  violation_count = 0;
  clamp_positive(-3000.0); // only pre(true) active, though x <= -1000
  if (violation_count != 0) __builtin_abort();

  // Member function template.
  Validator v;
  violation_count = 0;
  v.check(42); // integral: active, satisfied
  if (violation_count != 0) __builtin_abort();
  v.check(0.0); // not integral: discarded, though x == 0
  if (violation_count != 0) __builtin_abort();

  // Violated predicates with satisfied requires.
  violation_count = 0;
  ci.value = 100;
  ci.set(50); // pre active, violated (50 > 100 is false)
  if (violation_count != 1) __builtin_abort();

  violation_count = 0;
  Container<int> cn{-5};
  cn.get(); // post active, violated (-5 >= 0 is false)
  if (violation_count != 1) __builtin_abort();
}
