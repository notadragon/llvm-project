// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098 x P4283: a postcondition carrying a requires-clause, a capture and a
// result name, in the specified order `post requires(C) [captures]
// (result: pred)`.  The `[` after the constraint opens the capture list
// rather than subscripting the constraint.  A satisfied constraint keeps the
// capture and the check (one holds, one is violated, which also shows the
// capture holds the entry value); an unsatisfied one discards the whole
// postcondition.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-requires-capture.C

#include <concepts>
#include <contracts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++viol;
}

template <typename T>
T same(T x) post requires (std::integral<T>) [old = x] (r : r == old) {
  return x;
}

template <typename T>
T bump(T x) post requires (std::integral<T>) [old = x] (r : r == old) {
  return x + 1;
}

template <typename T>
T disc(T x) post requires (std::integral<T>) [old = x] (r : r == old) {
  return x + 1;
}

int main() {
  viol = 0;
  (void)same(5); // 5 == 5 -> holds
  if (viol != 0) __builtin_abort();
  (void)bump(5); // 6 == 5 -> violation (requires satisfied)
  if (viol != 1) __builtin_abort();
  (void)disc(5.0); // non-integral -> contract discarded, no check
  if (viol != 1) __builtin_abort();
}
