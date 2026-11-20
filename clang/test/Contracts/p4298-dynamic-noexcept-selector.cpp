// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-configuration-file=%S/p4298-dynamic-noexcept-selector.json %libcxx_flags -o %t && %t

// With -fcontracts-p4298, a P3595 dynamic selector may return noexcept_observe
// or noexcept_enforce, and the contract is evaluated with that semantic.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4298-dynamic-noexcept-selector.C
// in the gnu_gcc fork.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static evaluation_semantic chosen = evaluation_semantic::noexcept_observe;

extern "C" evaluation_semantic p4298_dynamic_noexcept_sel ()
{ return chosen; }

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation& v)
{
  if (v.semantic () == evaluation_semantic::noexcept_observe)
    ++violations;
  else if (v.semantic () == evaluation_semantic::noexcept_enforce)
    std::_Exit (0);                 // the program would be terminated
}

void f (int x) pre (x > 0) {}

int main () {
  f (-1);                           // noexcept_observe: continues
  if (violations != 1)
    __builtin_abort ();
  chosen = evaluation_semantic::noexcept_enforce;
  f (-1);                           // noexcept_enforce: the handler exits
  __builtin_abort ();
}

// REQUIRES: contracts-libcxx, native
