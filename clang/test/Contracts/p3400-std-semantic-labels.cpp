// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400 [support.contract.labels.sem]: the standard semantic restriction
// labels as contract labels.  The configured semantic is observe; each label
// adjusts it into its allowed set (D3595R1, "Adjusting a Semantic to the
// Allowed Set"): always_observe and never_enforce keep observe, symbolic and
// always_ignore make it ignore (not evaluated), and terminating,
// always_enforce and never_observe make it enforce (never_observe allows
// noexcept_observe, but that needs -fcontracts-p4298, so the adjustment
// moves up a level).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-std-semantic-labels.C
// in the gnu_gcc fork.

#include <contracts>
#include <cstdlib>

namespace L = std::contracts::labels;
using std::contracts::evaluation_semantic;

static int evaluated, observed, enforced;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() == evaluation_semantic::observe)
    ++observed;
  else if (v.semantic() == evaluation_semantic::enforce) {
    // Leaving the handler by an exception continues the program, so the
    // three enforced cases are counted rather than terminating.
    if (++enforced == 3)
      std::_Exit(evaluated == 5 && observed == 2 ? 0 : 1);
    throw 0;
  } else
    __builtin_abort();
}

bool chk(int x) {
  ++evaluated;
  return x > 0;
}

void a_obs(int x) pre<L::always_observe>(chk(x)) {}
void n_enf(int x) pre<L::never_enforce>(chk(x)) {}
void sym(int x) pre<L::symbolic>(chk(x)) {}
void a_ign(int x) pre<L::always_ignore>(chk(x)) {}
void term(int x) pre<L::terminating>(chk(x)) {}
void a_enf(int x) pre<L::always_enforce>(chk(x)) {}
void n_obs(int x) pre<L::never_observe>(chk(x)) {}

template <typename F> void enforced_call(F f) {
  try {
    f(-1);
  } catch (int) {
  }
}

int main() {
  a_obs(-1);
  n_enf(-1);
  sym(-1);
  a_ign(-1);
  if (evaluated != 2 || observed != 2)
    __builtin_abort();
  enforced_call(term);
  enforced_call(a_enf);
  enforced_call(n_obs); // the third enforced violation exits
  __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
