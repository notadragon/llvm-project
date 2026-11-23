// RUN: %clang -fcontracts-p4299 -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// A prototype with a contract specifier followed, in the same declaration,
// by a variable declarator is accepted.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-multi-declarator.c

int g(int a) _Pre(a > 0), x;
int g(int a) { return a; }
