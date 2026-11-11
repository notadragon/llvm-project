// RUN: %clang -fcontracts-p4299 -pedantic-errors %libcxx_flags %s -o %t && %t

// A GNU statement expression that declares a local is accepted in a _Pre or
// _Post predicate, and evaluated; on a prototype it is accepted too.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-stmt-expr-in-contract.c, and for
// the prototype open-bug-c-stmt-expr-in-prototype.c (which GCC
// rejects), in the gnu_gcc fork.

#include <contracts.h>

int f(int x) _Pre(__extension__({ int t = x; t > 0; })) { return x; }
int g(int x) _Post(r: __extension__({ int t = r; t >= 0; })) { return x; }

int h(int x) _Pre(__extension__({ int t = x; t > 0; }));

int main(void) { return f(1) + g(1) - 2; }

// REQUIRES: contracts-libcxx, native
