// P3100: a compute_semantic result of "assume" that is not in the allowed set
// is an error (not a silent downgrade).  Here -fcontracts-allow-assume is not
// given, so assume is not in the set.
// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

#include <contracts>
using namespace std::contracts;

// compute_semantic forces "assume", which is not in the allowed set.
struct to_assume_t {
  using assertion_control_object = to_assume_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::assume;
  }
};
constexpr to_assume_t to_assume{};

// CHECK: error: {{.*}}compute_semantic result is not in the allowed
void f(int x) pre<to_assume>(x > 0) {}
