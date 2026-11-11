// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P4283 x P3098: a postcondition with captures under a requires clause is
// discarded as a unit when the constraint is unsatisfied -- neither the captures
// nor the predicate are instantiated.  The predicate references a member (.val)
// that exists only when the constraint holds, so the unsatisfied instantiation
// would fail to compile if the postcondition were still instantiated.  No Clang
// bug found.
// (GCC mirror: g++.dg/contracts/cpp26/p4283-captures-discard.C.)

#include <contracts>

template <class T>
concept HasVal = requires(T t) { t.val; };

struct S { int val; };

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++viol;
}

// The predicate old.val is valid only under HasVal<T>; the capture takes the
// whole value.  When the constraint is unsatisfied the whole postcondition
// (capture + predicate) is discarded.
template <class T>
void f(T x)
  post requires (HasVal<T>) [old = x] (old.val >= 0)
{ }

int main() {
  // int has no .val: constraint unsatisfied -> postcondition discarded; must
  // compile (predicate never instantiated) and never fire.
  viol = 0; f(42);      if (viol != 0) __builtin_abort();

  // S satisfies HasVal: capture taken, predicate evaluated.
  viol = 0; f(S{-1});   if (viol != 1) __builtin_abort();  // old.val=-1 -> fails
  viol = 0; f(S{7});    if (viol != 0) __builtin_abort();  // old.val=7  -> holds
}
