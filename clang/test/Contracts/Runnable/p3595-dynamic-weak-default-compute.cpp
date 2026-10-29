// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-weak-default-compute.json \
// RUN:   %libcxx_flags -o %t && %t

// P3595: the weak default of a dynamic selector returns the semantic the
// configuration specifies, before it is adjusted to the allowed set and
// passed through the label's compute_semantic; the dynamic dispatch applies
// both to the value the selector returns, so they are applied once and the
// weak default reproduces the configured semantic's static behaviour, also
// for a compute_semantic that is not idempotent (D3595R1; CLANG-837 applied
// them twice).
//
// The configuration selects enforce, with a selector this program does not
// define.  compute_semantic maps enforce to observe and observe to ignore.
// Statically the contract is evaluated with observe (the handler is called
// once and the program continues), and so it is with the weak default.  A
// postcondition with captures is p3098-dynamic-capture-weak-compute.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-compute.C
#include <contracts>

using std::contracts::evaluation_semantic;

struct chain_t {
  using assertion_control_object = chain_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s == evaluation_semantic::enforce   ? evaluation_semantic::observe
           : s == evaluation_semantic::observe ? evaluation_semantic::ignore
                                               : s;
  }
};
constexpr chain_t chain{};

static int handled;
static evaluation_semantic seen;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++handled;
  seen = v.semantic();
}

void f(int x) pre<chain>(x > 0) {}

int main() {
  f(-1);
  if (handled != 1 || seen != evaluation_semantic::observe)
    __builtin_abort();
}
