// CE semantic: ignore.  Runtime semantic: enforce.
// Violated precondition at CE time must be silently ignored.
// RUN: %clang_cc1 -std=c++26 -fcontracts \
// RUN:   -fcontract-configuration-file=%S/ce-ignore-rt-enforce.json \
// RUN:   -fsyntax-only -verify %s

// expected-no-diagnostics

constexpr int f(int x) pre(x >= 0) { return x; }
constexpr int y = f(-1);
