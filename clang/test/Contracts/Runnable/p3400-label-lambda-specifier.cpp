// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t
// REQUIRES: contracts-libcxx

// P3400: a labelled function-contract-specifier can follow a lambda's
// lambda-specifier-seq.  gnu_gcc took `pre<label>` after `mutable`,
// `static` or `constexpr` for a template-id naming a type, and rejected
// the lambda ("expected '{'", its GCC-918); Clang did not.  Each label is
// applied: always_observe checks under the configured ignore.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-lambda-specifier.C

#include <contracts>

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template <evaluation_semantic S>
struct only {
  using assertion_control_object = only;
  static constexpr evaluation_semantic_set allowed_semantics{S};
};

static int violations;

void handle_contract_violation(const std::contracts::contract_violation &)
{
  ++violations;
}

auto l1 = [](int x) static pre<L::always_observe>(x > 0) { return x; };
auto l2 = [](int x) mutable pre<L::always_observe>(x > 0) { return x; };
auto l3 = [](int x) constexpr post<L::always_observe>(r: r > 0) { return x; };
auto l4 = [](int x) static constexpr pre <only<evaluation_semantic::observe>{}>(x > 0)
  post<L::always_observe>(r: r > 0) { return x; };
auto l5 = [](int x) mutable __pre<L::always_observe>(x > 0) { return x; };
auto l6 = [](int x) static -> int pre<L::always_observe>(x > 0) { return x; };
auto l7 = [](int x) static pre(x > 0) { return x; };

int main()
{
  l1(-1);
  l2(-1);
  l3(-1);
  l4(-1);
  l5(-1);
  l6(-1);
  l7(-1);
  if (violations != 7)
    __builtin_abort();
}
