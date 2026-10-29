// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-allow-assume -fcontract-configuration-file=%S/p3595-dynamic-assume-allowed.json %libcxx_flags -o %t && %t

// P3595 x P3100 dynamic selection: a compute_semantic result of "assume" is
// gated by -fcontracts-allow-assume.  WITH the flag, assume stays in the
// effective allowed set, so the per-value transform T(enforce)=assume resolves
// to assume, which codegens like ignore at the dynamic arm: no check, the
// predicate is not evaluated, and no violation fires.  Same source and config
// as the flag-off counterpart p3595-dynamic-assume-viol.cpp -- only the flag
// differs.
//
// %t is expected to exit 0: no handler call, predicate never evaluated.

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

struct to_assume_t {
  using assertion_control_object = to_assume_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set(evaluation_semantic::observe) |
      evaluation_semantic_set(evaluation_semantic::enforce) |
      evaluation_semantic_set(evaluation_semantic::assume);
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic __s) const {
    if (__s == evaluation_semantic::enforce)
      return evaluation_semantic::assume;
    return __s;
  }
};
constexpr to_assume_t to_assume{};

static int side = 0;
bool chk(int x) { ++side; return x > 0; }
// assume must not report a violation.
void handle_contract_violation(const contract_violation &) { std::abort(); }

evaluation_semantic p3595_ev_sel() { return evaluation_semantic::enforce; }

void f(int x) pre<to_assume>(chk(x)) {}

int main() {
  f(-1); // enforce -> assume (allowed): no check, predicate skipped
  if (side != 0)
    std::abort();
  return 0;
}
