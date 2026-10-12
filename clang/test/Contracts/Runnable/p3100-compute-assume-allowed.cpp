// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 \
// RUN:   -fcontracts-allow-assume -fcontract-evaluation-semantic=enforce \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3100: a compute_semantic result of "assume" that IS in the allowed set
// (via -fcontracts-allow-assume) survives resolution -- the contract resolves to
// assume, which currently codegens like ignore (no check, predicate not
// evaluated, no violation).  Positive counterpart to the not-allowed error case.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-compute-assume-allowed.C)

#include <contracts>

using std::contracts::evaluation_semantic;

struct to_assume_t {
  using assertion_control_object = to_assume_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::assume; }
};
constexpr to_assume_t to_assume{};

static int side = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const std::contracts::contract_violation&) {
  __builtin_abort();   // assume must not report a violation
}

void f(int x) pre<to_assume>(chk(x)) { }

int main() {
  f(-1);                       // resolves to assume -> no check, predicate skipped
  if (side != 0) __builtin_abort();
}
