// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint-ignore.json %libcxx_flags -o %t && %t

// P3100: float-cast range check across source (float/double) and destination
// integer types (signed/unsigned, narrow/wide).  ignore semantic: out-of-range
// -> 0, in-range unaffected (truncation toward zero).

#include <contracts>
#include <cstdlib>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++calls;
}

namespace ign_ns {
  int         to_i(double x) { return (int) x; }
  unsigned    to_u(double x) { return (unsigned) x; }
  short       to_s(double x) { return (short) x; }
  signed char to_c(float x)  { return (signed char) x; }
  long long   to_ll(double x) { return (long long) x; }
}

int main() {
  using namespace ign_ns;

  // int: 1e30 out of range -> 0; 5.9 -> 5.
  if (to_i(1e30) != 0) std::abort();
  if (to_i(5.9) != 5) std::abort();

  // unsigned: -1.0 out of range -> 0; 7.9 -> 7; -0.5 truncates to 0 (in range).
  if (to_u(-1.0) != 0) std::abort();
  if (to_u(7.9) != 7) std::abort();
  if (to_u(-0.5) != 0) std::abort();

  // short: 1e6 out of range -> 0; 100.9 -> 100; -100.9 -> -100.
  if (to_s(1e6) != 0) std::abort();
  if (to_s(100.9) != 100) std::abort();
  if (to_s(-100.9) != -100) std::abort();

  // signed char from float: 1e3 out of range -> 0; -100.9f -> -100.
  if (to_c(1e3f) != 0) std::abort();
  if (to_c(-100.9f) != -100) std::abort();

  // long long: 1e30 out of range (> LLONG_MAX) -> 0; 1e15 in range.
  if (to_ll(1e30) != 0) std::abort();
  if (to_ll(1e15) != 1000000000000000LL) std::abort();

  // ignore never calls the handler.
  if (calls != 0) std::abort();
}
