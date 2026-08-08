// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-overflow.json %libcxx_flags -o %t && %t

// P3100: signed integer overflow (ub:expr.expr.eval.signed.integer) for +, -, *.
// ignore -> the defined 2's-complement wrapped result, no handler.
// observe -> handler runs (assertion_kind::implicit), then continues with the
// wrapped result.  Unlike GCC (whose middle-end cannot host throwing observe
// and clamps it), Clang's codegen is in a valid EH context, so throwing observe
// is supported directly here.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-overflow.C, which uses
// -fcontracts-p4298 to reach noexcept_observe instead.)

#include <contracts>
#include <climits>
#include <cstdlib>

namespace cs = std::contracts;

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation(const cs::contract_violation& v) {
  ++calls;
  if (v.kind() != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  int add(int a, int b) { return a + b; }
  int mul(int a, int b) { return a * b; }
}
namespace obs_ns {                    // observe
  int add(int a, int b) { return a + b; }
}

int main() {
  // ignore: defined wrapped result, no handler.
  if (ign_ns::add(INT_MAX, 1) != INT_MIN) std::abort();
  if (ign_ns::mul(INT_MAX, 2) != -2) std::abort();
  if (calls != 0) std::abort();

  // observe: handler runs, continues with the wrapped result.
  if (obs_ns::add(INT_MAX, 1) != INT_MIN) std::abort();
  if (calls != 1) std::abort();
  if (!all_implicit) std::abort();

  // no overflow: normal result, no handler either way.
  if (ign_ns::add(2, 3) != 5) std::abort();
  if (obs_ns::add(2, 3) != 5) std::abort();
  if (calls != 1) std::abort();
}
