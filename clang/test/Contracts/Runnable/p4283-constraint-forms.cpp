// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P4283: a requires clause on a contract assertion may carry any
// constraint-logical-or-expression, not just a single concept-check --
// conjunction, disjunction, negation, and a lone atomic constraint all select
// or discard the contract per the satisfaction result.  The parentheses
// that delimit the requires-clause are enough for a negation or a comparison.
// (GCC mirror: g++.dg/contracts/cpp26/p4283-constraint-forms.C.)

#include <concepts>
#include <contracts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++viol;
}

// Conjunction: active only when BOTH concepts hold.
template <class T> void f_and(T x)
  pre requires (std::integral<T> && std::signed_integral<T>) (x > 0) { }

// Disjunction.
template <class T> void f_or(T x)
  pre requires (std::integral<T> || std::floating_point<T>) (x > 0) { }

// Negation.
template <class T> void f_not(T x)
  pre requires (!std::integral<T>) (x > 0) { }

// Lone atomic (non-concept) constraint.
template <class T> void f_sz(T x)
  pre requires (sizeof(T) >= 4) (x > 0) { }

// A single concept-check.
template <class T> void f_one(T x)
  pre requires (std::integral<T>) (x > 0) { }

// Compound requires on an auto-return postcondition.
template <class T> auto f_auto(T x)
  post requires (std::integral<T> && std::signed_integral<T>) (r: r > 0)
{ return x; }

int main() {
  viol = 0; f_and((int)-1);       if (viol != 1) __builtin_abort(); // both hold
  viol = 0; f_and((unsigned)0u);  if (viol != 0) __builtin_abort(); // not signed
  viol = 0; f_and((double)-1.0);  if (viol != 0) __builtin_abort(); // not integral

  viol = 0; f_or((int)-1);        if (viol != 1) __builtin_abort(); // integral
  viol = 0; f_or((double)-1.0);   if (viol != 1) __builtin_abort(); // floating

  viol = 0; f_not((double)-1.0);  if (viol != 1) __builtin_abort(); // !integral true
  viol = 0; f_not((int)-1);       if (viol != 0) __builtin_abort(); // !integral false

  viol = 0; f_sz((int)-1);        if (viol != 1) __builtin_abort(); // sizeof>=4
  viol = 0; f_sz((char)-1);       if (viol != 0) __builtin_abort(); // sizeof<4

  viol = 0; f_one((int)-1);       if (viol != 1) __builtin_abort();
  viol = 0; f_one((double)-1.0);  if (viol != 0) __builtin_abort();

  viol = 0; f_auto((int)-1);      if (viol != 1) __builtin_abort(); // active
  viol = 0; f_auto((unsigned)0u); if (viol != 0) __builtin_abort(); // discarded
}
