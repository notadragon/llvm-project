// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -O1 -fcontract-configuration-file=%S/p3100-throw-bounds-divoverflow.json %libcxx_flags -o %t && %t

// P3100: a throwing handler under the front-end array-bounds
// (ub:expr.add.out.of.bounds.known) and division-overflow
// (ub:expr.mul.representable.type.result) checks, in functions that are not
// noexcept: the exception propagates out of the checked operation and is
// caught by the caller, under observe and enforce alike.  The division
// overflow check had no throwing-handler test;
// p3100-array-bounds-throw-nonnoexcept.cpp covers array bounds at -O0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-throw-bounds-divoverflow.C)

#include <contracts>
#include <cstdlib>

struct E {
  int tag;
};
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++calls;
  if (v.kind() != std::contracts::assertion_kind::implicit)
    std::abort();
  throw E{7};
}

int g[4];
constexpr int kMin = -__INT_MAX__ - 1;
namespace obs_ns {
int rd(int i) { return g[i]; }
int divi(int a, int b) { return a / b; }
int modi(int a, int b) { return a % b; }
} // namespace obs_ns
namespace enf_ns {
int rd(int i) { return g[i]; }
int divi(int a, int b) { return a / b; }
} // namespace enf_ns

__attribute__((noinline)) int opaque(int x) { return x; }

template <class F> static int caught(F f) {
  try {
    f();
  } catch (E &e) {
    return e.tag == 7;
  }
  return 0;
}

int main() {
  int n = 0;
  n += caught([] { obs_ns::rd(opaque(4)); });
  n += caught([] { obs_ns::divi(kMin, opaque(-1)); });
  n += caught([] { obs_ns::modi(kMin, opaque(-1)); });
  n += caught([] { enf_ns::rd(opaque(-1)); });
  n += caught([] { enf_ns::divi(kMin, opaque(-1)); });
  if (n != 5 || calls != 5)
    std::abort();
  // In range: no handler.
  obs_ns::rd(opaque(3));
  enf_ns::divi(kMin, opaque(1));
  if (calls != 5)
    std::abort();
}
