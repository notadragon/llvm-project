// P4283 x P3097: a requires-clause on a virtual function's contract is honored
// through the interface/implementation split.  When the constraint is
// unsatisfied for an instantiation the contract is discarded and the P3097
// interface wrapper omits it, so a call through the interface fires nothing;
// when the constraint is satisfied the contract is enforced as usual.
// Mirrors GCC's g++.dg/contracts/cpp26/p4283-virtual-discard.C.
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3097 -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

#include <concepts>
#include <contracts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++viol;
}

template <class T>
struct Base {
  virtual void f(T x) pre requires (std::integral<T>) (x > 0) {}
  virtual ~Base() = default;
};

int main() {
  Base<int> bi;
  Base<double> bd;
  Base<int> *pi = &bi;
  Base<double> *pd = &bd;

  // integral: constraint satisfied -> contract active -> a violating call fires
  // (through the P3097 interface + implementation checks).
  viol = 0;
  pi->f(-1);
  if (viol == 0) __builtin_abort();

  // double: constraint unsatisfied -> contract discarded -> the interface
  // wrapper omits it -> a violating call fires nothing.
  viol = 0;
  pd->f(-1.0);
  if (viol != 0) __builtin_abort();

  return 0;
}
