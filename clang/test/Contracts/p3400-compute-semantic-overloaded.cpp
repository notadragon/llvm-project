// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// P3400: a public compute_semantic facet overloaded with a private
// tag-dispatch helper of the same name is applied (enforce becomes observe),
// whichever of the two is declared first.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-overloaded.C

#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct tag {};
struct A { // public facet first, private helper second
  using assertion_control_object = A;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return compute_semantic(tag{}, s);
  }

private:
  constexpr evaluation_semantic compute_semantic(tag,
                                                 evaluation_semantic) const {
    return evaluation_semantic::observe;
  }
};
struct B { // private helper first, public facet second
  using assertion_control_object = B;

private:
  constexpr evaluation_semantic compute_semantic(tag,
                                                 evaluation_semantic) const {
    return evaluation_semantic::observe;
  }

public:
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return compute_semantic(tag{}, s);
  }
};
constexpr A a{};
constexpr B b{};
static_assert(std::contracts::labels::semantic_computation_label<A>);
static_assert(std::contracts::labels::semantic_computation_label<B>);

void f(int x) pre<a>(x > 0) {}
void g(int x) pre<b>(x > 0) {}

int main() {
  f(-1);
  g(-1);
  return violations == 2 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
