// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-weak-default-shared-label.json \
// RUN:   %libcxx_flags -o %t && %t

// P3595: two contracts with different labels share one dynamic selector,
// which the program does not define.  The weak definition is one per
// symbol, the label per contract, so the weak default returns the
// configured semantic, which depends on neither label, and each check
// passes it through its own label (D3595R1).  CLANG-837 returned the first
// contract's semantic after its label, here ignore, so the second, with no
// label, was ignored too.
//
// The configuration selects observe.  f_a's label maps observe to ignore,
// so f_a is not checked; f_b has no label and is checked with observe.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-shared-label.C
#include <contracts>

using std::contracts::evaluation_semantic;

extern "C" evaluation_semantic shared_label_weak_sel();

struct to_ignore_t {
  using assertion_control_object = to_ignore_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s == evaluation_semantic::observe ? evaluation_semantic::ignore : s;
  }
};
constexpr to_ignore_t to_ignore{};

static int handled;
static evaluation_semantic seen;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++handled;
  seen = v.semantic();
}

// f_a is defined, and its check emitted, first.
void f_a(int x) pre<to_ignore>(x > 0) {}
void f_b(int x) pre(x > 0) {}

int main() {
  if (shared_label_weak_sel() != evaluation_semantic::observe)
    __builtin_abort();
  f_a(-1);
  if (handled != 0)
    __builtin_abort();
  f_b(-1);
  if (handled != 1 || seen != evaluation_semantic::observe)
    __builtin_abort();
}
