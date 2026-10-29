// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 \
// RUN:   -fcontract-configuration-file=%S/p4283-config.json \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P4283 x P3595: configuration and the requires-clause are independent.  A
// constrained contract assertion whose constraint is satisfied takes the
// semantic the configuration selects for it (observe, in namespace obs);
// one whose constraint is not satisfied is discarded whatever the
// configuration says (enforce, in namespace enf, would terminate).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-config.C

#include <concepts>
#include <contracts>

static int violations;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  if (v.semantic() != std::contracts::evaluation_semantic::observe)
    __builtin_abort();
}

namespace obs {
template <class T> void f(T x) pre requires (std::integral<T>) (x > 0) {}
} // namespace obs
namespace enf {
template <class T> void f(T x) pre requires (std::integral<T>) (x > 0) {}
template <class T> void g(T x) {
  contract_assert requires (std::integral<T>) (x > 0);
}
} // namespace enf

int main() {
  obs::f(0);   // kept, observed
  obs::f(0.0); // discarded
  if (violations != 1)
    __builtin_abort();
  enf::f(0.0); // discarded: no termination
  enf::g(0.0);
  enf::f(1); // kept, holds
  if (violations != 1)
    __builtin_abort();
}
