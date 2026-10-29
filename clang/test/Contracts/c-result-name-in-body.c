// RUN: %clang -fcontracts-p4299 -fsyntax-only -Xclang -verify %s

// The _Post result name is scoped to the postcondition predicate (P4299):
// the body cannot name it.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-result-name-in-body.c

int f(void) _Post(r: r > 0) {
  return r; // expected-error {{use of undeclared identifier 'r'}}
}
