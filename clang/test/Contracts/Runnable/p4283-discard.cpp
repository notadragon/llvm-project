// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P4283: Unsatisfied constraints discard contracts at instantiation.

#include <contracts>
#include <cstdio>

template<typename T>
concept Integral = __is_integral(T);

template<typename T>
concept SignedIntegral = Integral<T> && __is_signed(T);

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

template<typename T>
T safe_divide(T a, T b)
  pre requires(Integral<T>) (b != 0)
{
  if (b == T{}) return T{};
  return a / b;
}

template<typename T>
void check_positive(T x) {
  contract_assert requires(SignedIntegral<T>) (x > 0);
}

int main() {
  // int: constraint satisfied, b==0 triggers violation.
  violation_count = 0;
  safe_divide(10, 0);
  if (violation_count != 1) __builtin_abort();

  // double: constraint NOT satisfied (not integral), contract discarded.
  violation_count = 0;
  safe_divide(10.0, 0.0);
  if (violation_count != 0) __builtin_abort();  // no violation!

  // signed int: constraint satisfied, x<0 triggers violation.
  violation_count = 0;
  check_positive(-1);
  if (violation_count != 1) __builtin_abort();

  // unsigned: constraint NOT satisfied, contract discarded.
  violation_count = 0;
  check_positive(0u);
  if (violation_count != 0) __builtin_abort();

  // double: constraint NOT satisfied (not signed_integral), discarded.
  violation_count = 0;
  check_positive(-1.0);
  if (violation_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
