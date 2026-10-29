// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fno-contracts-prove-assume -fcontracts-prove-assume -fcontract-evaluation-semantic=diagnose %libcxx_flags -o %t -Xclang -verify
// RUN: %t

// P4405: -fcontracts-prove-assume (implied by -fcontracts-p4405) is
// accepted, as is its -fno- form, and with folding as the only strategy it
// changes nothing: a proven predicate is still not evaluated and adds no
// check, and an unproven one is still diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-assume.C (GCC
// proves x == x by folding; Clang does not)

#include <contracts>

static int evaluated, violations;
constexpr bool always() { return true; }
bool chk(int x) { ++evaluated; return x > 0; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x)
  pre(always())                 // proven
  pre(chk(x))                   // expected-note {{the predicate of the contract assertion}}
{
  contract_assert(x == x);      // expected-warning {{contract assertion cannot be proven}} \
                                // expected-warning {{self-comparison always evaluates to true}}
  contract_assert(false);       // expected-warning {{contract predicate is always false}}
  return x;
}

int main() {
  if (f(-1) != -1 || evaluated != 0 || violations != 0) // expected-warning {{contract assertion cannot be proven}}
    __builtin_abort();
}
