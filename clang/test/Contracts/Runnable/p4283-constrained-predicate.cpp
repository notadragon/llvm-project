// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P4283 motivating case: a precondition whose predicate names something
// valid only when the constraint holds (here a member `.val` that exists only
// for class types satisfying HasVal).  When the constraint is not satisfied
// at instantiation the whole assertion is discarded and its predicate is
// never instantiated, so f(42) compiles even though `int` has no `.val`.
// (p4283-captures-discard.cpp covers the same for a postcondition with a
// capture.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-constrained-predicate.C

#include <contracts>

template <typename T> concept HasVal = requires(T t) { t.val; };

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++viol;
}

template <typename T> void f(T x) pre requires (HasVal<T>) (x.val > 0) {}

struct WithVal { int val; };

int main() {
  viol = 0;
  f(WithVal{1}); // constraint satisfied, predicate valid, holds
  if (viol != 0) __builtin_abort();
  f(WithVal{-1}); // satisfied, predicate false -> violation
  if (viol != 1) __builtin_abort();
  f(42); // NOT satisfied -> assertion discarded; x.val never formed
  if (viol != 1) __builtin_abort();
}
