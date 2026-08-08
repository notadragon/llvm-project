// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-fpint-ignore.json %libcxx_flags -o %t && %t

// P3100: the float-cast guard evaluates its operand exactly once, and the
// operand's side effects still happen on the violation path.  ignore semantic.

#include <contracts>
#include <cstdlib>

static int f_calls = 0;
double F(double v) { ++f_calls; return v; }

namespace ign_ns {
  int fi(double x) { return (int) F(x); }
}

int main() {
  // Violation path (out of range): operand evaluated exactly once.
  f_calls = 0;
  if (ign_ns::fi(1e30) != 0) std::abort();
  if (f_calls != 1) std::abort();

  // Normal path: operand evaluated exactly once.
  f_calls = 0;
  if (ign_ns::fi(42.9) != 42) std::abort();
  if (f_calls != 1) std::abort();
}
