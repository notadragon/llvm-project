// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-allowed.json %libcxx_flags -o %t && %t

// P3595 dynamic selection with an allowed_semantics label.  The label allows
// only {observe}.  The selector returns "enforce", which is clamped to the
// allowed set -> "observe".  The failing precondition is therefore handled and
// execution continues (violations == 1), rather than terminating.

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

struct observe_only_t {
  using assertion_control_object = observe_only_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set(evaluation_semantic::observe);
};
constexpr observe_only_t observe_only{};

static int violations = 0;

void handle_contract_violation(const contract_violation &) { ++violations; }

evaluation_semantic p3595_allowed_sel() {
  return evaluation_semantic::enforce;
}

void f(const int x) pre<observe_only>(x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    std::abort();
  f(1);
  if (violations != 1)
    std::abort();
}
