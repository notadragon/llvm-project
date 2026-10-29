// CE semantic: enforce.  Runtime semantic: ignore.
// Violated precondition in manifestly-CE context must be an error.
// RUN: %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-enforce-rt-ignore.json \
// RUN:   -fsyntax-only -verify %s
// RUN: %clangxx -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-enforce-rt-ignore.json \
// RUN:   -DRUNTIME %libcxx_flags %s -o %t
// RUN: %t

constexpr int f(int x) pre(x >= 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

#ifndef RUNTIME
constexpr int y = f(-1); // expected-error {{constexpr variable 'y' must be initialized by a constant expression}}
  // expected-note@-1 {{in call to 'f(-1)'}}
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
  return calls; // runtime ignore: no handler call
}
#endif

// REQUIRES: contracts-libcxx, native
