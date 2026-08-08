// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: a postcondition capture whose initializer is a type-dependent
// expression -- e.g. a member access on a dependent function parameter,
// [c = x.val] -- must not crash.  The initializer's type is unknown at parse
// time, so the capture's type is deduced at instantiation from the substituted
// initializer.  Regression: this previously asserted in CodeGen ("Unknown
// builtin type" -- the dependent placeholder reached getTypeInfoImpl) whenever
// the enclosing function was a template.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-member-template.C.)

#include <contracts>

struct S { int val; };

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++viol;
}

// Capture initializer x.val is a member access on the dependent parameter x.
template <class T>
void f(T x)
  post [c = x.val] (c >= 0)
{ }

// Also exercise that the value is captured (not re-read from x) by mutating the
// object after entry -- the capture holds the entry-time value.
template <class T>
void g(T x)
  post [c = x.val] (c >= 0)
{ x.val = 999; }

int main() {
  viol = 0; f(S{5});   if (viol != 0) __builtin_abort();  // c = 5  -> holds
  viol = 0; f(S{-3});  if (viol != 1) __builtin_abort();  // c = -3 -> fails

  viol = 0; g(S{7});   if (viol != 0) __builtin_abort();  // c = 7 (entry value)
  viol = 0; g(S{-1});  if (viol != 1) __builtin_abort();  // c = -1 -> fails
}
