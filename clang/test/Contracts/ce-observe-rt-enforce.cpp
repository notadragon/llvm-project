// CE semantic: observe (warning only, evaluation continues).
// Runtime semantic: enforce.
// RUN: %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-observe-rt-enforce.json \
// RUN:   -fsyntax-only -verify %s

constexpr int f(int x) pre(x >= 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}

// Observe semantic: contract violation is diagnosed as a warning but
// evaluation continues -- the variable is successfully initialized.
constexpr int y = f(-1);
