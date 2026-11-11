// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-allow-assume -fcontract-configuration-file=%S/p3595-dynamic-weak-default-assume.json %libcxx_flags -o %t && %t

// With -fcontracts-allow-assume, a dynamically selected contract whose
// configuration names assume as the default semantic gets a weak default
// selector that returns assume.  A selector may return assume: allowed, it
// takes the ignore path, so the predicate is not evaluated and nothing is
// reported (notadragon_wg21 DECISIONS-resolved.md E66; CLANG-673 was an
// enforced violation here).  Without the flag assume is adjusted to ignore
// (p3595-dynamic-adjust-selector.cpp).  The C form is
// C/c-dynamic-weak-default-assume.c, which shares this configuration file.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-weak-default-assume.C
// in the gnu_gcc fork.

#include <contracts>

int violations, evaluated;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

bool chk(int x) {
  ++evaluated;
  return x > 0;
}

int f(int x) pre(chk(x)) { return x; }

int main() {
  f(-1);
  return violations + evaluated;
}

// REQUIRES: contracts-libcxx, native
