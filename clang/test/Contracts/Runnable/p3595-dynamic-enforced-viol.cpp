// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-enforced-viol.json %libcxx_flags -o %t && %t

// P3595 dynamic selection: a per-value transform that lands outside the allowed
// set becomes a RUNTIME enforced violation (not a compile error).  The label
// allows {observe, enforce} and maps enforce -> quick_enforce, but quick_enforce
// is NOT in the allowed set, so T(enforce) is the sentinel.
//
// The selector returns "enforce", so the dispatch reaches the sentinel arm.
// That arm fires the enforced violation UNCONDITIONALLY -- it does not evaluate
// the predicate.  The violation handler runs first and calls std::exit(0), so
// the process exits 0 before the (noreturn) enforce terminate is observable and
// before main's std::abort().
//
// RUN-line decision: %t is expected to exit 0 (handler's std::exit(0) wins).

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

struct enforce_to_quick_t {
  using assertion_control_object = enforce_to_quick_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set(evaluation_semantic::observe) |
      evaluation_semantic_set(evaluation_semantic::enforce);
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic __s) const {
    if (__s == evaluation_semantic::enforce)
      return evaluation_semantic::quick_enforce;
    return __s;
  }
};
constexpr enforce_to_quick_t enforce_to_quick{};

void handle_contract_violation(const contract_violation &) { std::exit(0); }

evaluation_semantic p3595_ev_sel() { return evaluation_semantic::enforce; }

void f(const int x) pre<enforce_to_quick>(x > 0) {}

int main() {
  // Predicate is satisfied (1 > 0), but the sentinel arm does not test it: the
  // enforced violation fires regardless, the handler runs std::exit(0).
  f(1);
  std::abort();
}
