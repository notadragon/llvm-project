// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags -o %t && %t

// P3100 x P3400 x templates: the assume semantic is gated by the label's
// allowed set per instantiation.  The configured semantic is assume; the
// label is a variable template whose allowed_semantics depends on the
// function template's parameter, so int's instantiation allows assume (the
// predicate is never evaluated) while long long's does not and adjusts to
// observe (evaluated and reported).  A class template's member shows the
// same per instantiation of the class.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-template-allowed-set.C)

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template <class T> struct gate_t {
  using assertion_control_object = gate_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      sizeof(T) <= 4
          ? evaluation_semantic_set{evaluation_semantic::assume,
                                    evaluation_semantic::observe}
          : evaluation_semantic_set{evaluation_semantic::observe};
};
template <class T> constexpr gate_t<T> gate{};

static int evaluated = 0, violations = 0;
static bool chk(long long x) {
  ++evaluated;
  return x > 0;
}
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template <class T> void f(T x) pre<gate<T>>(chk(x)) {}

template <class T> struct S {
  void g(int x) pre<gate<T>>(chk(x)) {}
};

int main() {
  f(-1); // int: assume allowed, not evaluated
  S<char>().g(-1);
  if (evaluated != 0 || violations != 0)
    std::abort();
  f(-1LL); // long long: assume not allowed -> observe
  if (evaluated != 1 || violations != 1)
    std::abort();
  S<long long>().g(-1);
  if (evaluated != 2 || violations != 2)
    std::abort();
}
