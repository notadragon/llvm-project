// CE semantic: enforce.  Runtime semantic: ignore.
// Violated precondition in manifestly-CE context must be an error.
// RUN: %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-enforce-rt-ignore.json \
// RUN:   -fsyntax-only -verify %s

constexpr int f(int x) pre(x >= 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

constexpr int y = f(-1); // expected-error {{constexpr variable 'y' must be initialized by a constant expression}}
  // expected-note@-1 {{in call to 'f(-1)'}}
