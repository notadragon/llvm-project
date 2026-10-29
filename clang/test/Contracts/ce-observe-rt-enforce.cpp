// CE semantic: observe (warning only, evaluation continues).
// Runtime semantic: enforce.
// RUN: %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-observe-rt-enforce.json \
// RUN:   -fsyntax-only -verify %s
// RUN: %clangxx -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-observe-rt-enforce.json \
// RUN:   -DRUNTIME %libcxx_flags %s -o %t
// RUN: not --crash %t 2>&1 | FileCheck %s --check-prefix=RT

constexpr int f(int x) pre(x >= 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}

// Observe semantic: contract violation is diagnosed as a warning but
// evaluation continues -- the variable is successfully initialized.
#ifndef RUNTIME
constexpr int y = f(-1);
#endif

// The runtime half: the same precondition, violated outside constant
// evaluation, follows the runtime semantic.
#ifdef RUNTIME
#include <contracts>
#include <cstdio>
static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
  std::fputs("runtime handler ran\n", stderr);
}
int main() {
  volatile int v = -1;
  f(v);
  std::fputs("continued after the violation\n", stderr);
  return 0;
}
#endif
// RT: runtime handler ran
// RT-NOT: continued after the violation

// REQUIRES: contracts-libcxx, native
