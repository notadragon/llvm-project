// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-nodefault.json %libcxx_flags -o %t && %t

// P3595: a "dynamic" entry with no "semantic" and "provideweak": false --
// the user supplies the only definition of the selector, and a
// definition-side contract uses it.  (GCC mirror:
// g++.dg/contracts/cpp26/p3595-dynamic-nodefault.C)

#include <contracts>

using std::contracts::evaluation_semantic;

int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// The only definition of the selector: provideweak is false and no
// "semantic" was given, so the compiler emits nothing for it.
evaluation_semantic
p3595_sel_nd ()
{
  return evaluation_semantic::observe;
}

// Definition-side contract: this is the side that hard-errors on CES_INVALID.
void
f (const int x) pre (x > 0)
{
}

int
main ()
{
  f (-1);
  if (violations != 1)
    __builtin_abort ();
  return 0;
}
