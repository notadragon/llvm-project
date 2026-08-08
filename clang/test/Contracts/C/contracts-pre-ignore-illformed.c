// An ignored precondition is still parsed, so an ill-formed predicate is
// diagnosed even under the ignore semantic (D4299).
// RUN: %clang_cc1 -fcontracts-p4299 -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-pre-ignore-illformed.c.)

int foo(int x) _Pre(undeclared_var > 0) // expected-error{{undeclared}}
{
  return x;
}
