// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds.json %libcxx_flags -o %t && %t

// P3100: array subscript out of bounds with a statically-known bound
// (ub:expr.add.out.of.bounds.known).  ignore -> the subscript is redirected to the
// valid index 0 (defined for reads and writes), no handler; observe -> handler
// runs (assertion_kind::implicit) then the access uses index 0.  Bounds is a
// front-end check, so throwing observe is supported directly.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds.C)

#include <contracts>
#include <cstdlib>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
}

int g[4] = { 10, 20, 30, 40 };
namespace ign_ns {                    // ignore
  int rd(int i) { return g[i]; }
  void wr(int i, int v) { g[i] = v; }
}
namespace obs_ns {                    // observe
  int rd(int i) { return g[i]; }
}

__attribute__((noinline)) int opaque(int x) { return x; }

int main() {
  if (ign_ns::rd(opaque(100)) != 10) std::abort();   // g[0]
  if (ign_ns::rd(opaque(-1)) != 10) std::abort();    // negative -> g[0]
  if (ign_ns::rd(opaque(2)) != 30) std::abort();     // in bounds
  if (calls != 0) std::abort();

  ign_ns::wr(opaque(100), 99);                       // OOB write -> g[0]
  if (g[0] != 99) std::abort();
  g[0] = 10;

  if (obs_ns::rd(opaque(100)) != 10) std::abort();
  if (calls != 1) std::abort();
  if (!all_implicit) std::abort();
}
