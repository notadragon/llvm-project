// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -o %t && %t

// evaluation_semantic_set::all(), ~ and the never_X labels cover the
// extension semantics (notadragon_wg21 DECISIONS-resolved.md E65 2a), so a
// label built from them admits noexcept_observe under -fcontracts-p4298: the
// configured noexcept_observe is kept, where a set of the standard four
// alone would adjust it to observe.  never_observe excludes only observe.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-std-labels-extensions.C
// in the gnu_gcc fork.

#include <contracts>

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

static int nx_observed, observed;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() == evaluation_semantic::noexcept_observe)
    ++nx_observed;
  else if (v.semantic() == evaluation_semantic::observe)
    ++observed;
  else
    __builtin_abort();
}

struct all_t {
  using assertion_control_object = all_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set::all();
};
constexpr all_t all_label{};

struct complement_t {
  using assertion_control_object = complement_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      ~evaluation_semantic_set{evaluation_semantic::enforce};
};
constexpr complement_t complement_label{};

struct standard_four_t {
  using assertion_control_object = standard_four_t;
  static constexpr evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::ignore, evaluation_semantic::observe,
      evaluation_semantic::enforce, evaluation_semantic::quick_enforce};
};
constexpr standard_four_t standard_four{};

void f1(int x) pre<all_label>(x > 0) {}
void f2(int x) pre<complement_label>(x > 0) {}
void f3(int x) pre<L::never_observe>(x > 0) {}
void f4(int x) pre<standard_four>(x > 0) {}

int main() {
  f1(-1);
  f2(-1);
  f3(-1);
  if (nx_observed != 3 || observed != 0)
    __builtin_abort();
  f4(-1); // adjusted to observe
  if (nx_observed != 3 || observed != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
