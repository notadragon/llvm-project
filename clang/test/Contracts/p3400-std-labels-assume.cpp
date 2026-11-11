// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags -o %t && %t

// evaluation_semantic_set::all() covers assume (notadragon_wg21
// DECISIONS-resolved.md E65 2a), so under -fcontracts-allow-assume a label
// whose allowed set is all() keeps a configured assume, and its
// compute_semantic is handed assume; a label allowing only the standard four
// adjusts assume to ignore first.  Each label's compute_semantic maps assume
// to observe and anything else to ignore, so only the first reports the
// violation.  never_ignore allows assume too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-std-labels-assume.C in
// the gnu_gcc fork.

#include <contracts>

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

static_assert(
    L::never_ignore.allowed_semantics.contains(evaluation_semantic::assume));

static int observed;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() == evaluation_semantic::observe)
    ++observed;
  else
    __builtin_abort();
}

template <bool All> struct probe_t {
  using assertion_control_object = probe_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      All ? evaluation_semantic_set::all()
          : evaluation_semantic_set{
                evaluation_semantic::ignore, evaluation_semantic::observe,
                evaluation_semantic::enforce,
                evaluation_semantic::quick_enforce};
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s == evaluation_semantic::assume ? evaluation_semantic::observe
                                            : evaluation_semantic::ignore;
  }
};
constexpr probe_t<true> all_probe{};
constexpr probe_t<false> four_probe{};

void f1(int x) pre<all_probe>(x > 0) {}
void f2(int x) pre<four_probe>(x > 0) {}

int main() {
  f1(-1);
  if (observed != 1)
    __builtin_abort();
  f2(-1);
  if (observed != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
