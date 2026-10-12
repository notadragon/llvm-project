// RUN: %clang -fcontracts-p4299 -fcontracts-allow-assume -Werror=unused-command-line-argument -fsyntax-only %s

// -fcontracts-allow-assume is honoured in C: it reaches cc1 and defines
// __clang_contracts_allow_assume.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-allow-assume-macro.c

#include <contracts.h>

#ifndef __clang_contracts_allow_assume
#error "__clang_contracts_allow_assume not defined for C"
#endif

int f(int x) _Pre(x > 0) { return x; }
