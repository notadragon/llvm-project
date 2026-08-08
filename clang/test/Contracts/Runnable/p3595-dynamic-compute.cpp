// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-compute.json %libcxx_flags -o %t && %t

// P3595 dynamic selection with a compute_semantic label.  The label maps
// enforce/quick_enforce -> observe (leaving other semantics unchanged).  The
// selector returns "enforce"; the per-value transform yields observe, so the
// failing precondition is handled and execution continues (violations == 1).

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

struct to_observe_t {
  using assertion_control_object = to_observe_t;
  constexpr evaluation_semantic
  compute_semantic(evaluation_semantic __s) const {
    if (__s == evaluation_semantic::enforce ||
        __s == evaluation_semantic::quick_enforce)
      return evaluation_semantic::observe;
    return __s;
  }
};
constexpr to_observe_t to_observe{};

static int violations = 0;

void handle_contract_violation(const contract_violation &) { ++violations; }

evaluation_semantic p3595_compute_sel() {
  return evaluation_semantic::enforce;
}

void f(const int x) pre<to_observe>(x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    std::abort();
  f(1);
  if (violations != 1)
    std::abort();
}
