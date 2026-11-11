// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-assume-viol.json %libcxx_flags -o %t && %t

// P3595 x P3100 dynamic selection: a compute_semantic result of "assume" is
// gated by -fcontracts-allow-assume.  WITHOUT the flag, assume is removed from
// the effective allowed set, so the per-value transform T(enforce)=assume lands
// outside the set and becomes the sentinel -- an unconditional RUNTIME enforced
// violation (not a compile error), even though the predicate is satisfied.  The
// label lists assume as allowed, so only the missing flag gates it out.
// Flag-on counterpart: p3595-dynamic-assume-allowed.cpp.
//
// %t is expected to exit 0 (handler's std::exit(0) wins before the abort()).

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

void handle_contract_violation(const contract_violation &) { std::exit(0); }

evaluation_semantic p3595_ev_sel() { return evaluation_semantic::enforce; }

void f(const int x) pre<to_assume>(x > 0) {}

int main() {
  // Predicate satisfied (1 > 0), but assume is gated out, so the sentinel arm
  // fires the enforced violation unconditionally; the handler runs std::exit(0).
  f(1);
  std::abort();
}
