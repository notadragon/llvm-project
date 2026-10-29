// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A private compute_semantic is not a facet -- the library concept is false
// for it -- wherever the contract is written: access is judged from outside
// the label, so inside a friend of the label the configured observe stays
// observe, as on a namespace-scope function.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/facet-access-from-friend.C

#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct L {
  using assertion_control_object = L;

private:
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::enforce;
  }
  friend struct User;
};
constexpr L lbl{};
static_assert(!std::contracts::labels::semantic_computation_label<L>);

struct User {
  static void f(int x) pre<lbl>(x > 0) {}
};
void g(int x) pre<lbl>(x > 0) {}

int main() {
  g(-1);
  User::f(-1);
  return violations == 2 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
