// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s

// P4405 stage 2 in C, -fcontracts-prove-fold (implied by -fcontracts-p4405):
// an evaluation with the prove semantic whose predicate folds to true is
// proven and not diagnosed, whether or not it is a constant expression; one
// that folds to false is "always false"; anything else still "cannot be
// proven".  A postcondition is diagnosed once.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-fold-prove.c (whose folds
// of non-constant expressions, x == x, u >= 0u, r == r and x != x, are
// GCC's: Clang leaves them unproven; the proof shadow GCC switches off there
// does not exist in Clang)

int f(int x, unsigned u)
  _Pre(1)
  _Pre(sizeof(int) >= 1)
  _Pre(x == x)                  // expected-error {{contract assertion cannot be proven}} \
                                // expected-warning {{self-comparison always evaluates to true}}
  _Pre(u >= 0u)                 // expected-error {{contract assertion cannot be proven}}
  _Pre(x > 0)                   // expected-error {{contract assertion cannot be proven}}
  _Post(r: r == r)              // expected-error {{contract assertion cannot be proven}} \
                                // expected-warning {{self-comparison always evaluates to true}}
  _Post(r: r > 0)               // expected-error {{contract assertion cannot be proven}}
{
  _ContractAssert(1 + 1 == 2);
  _ContractAssert(0);           // expected-error {{contract predicate is always false}}
  _ContractAssert(x != x);      // expected-error {{contract assertion cannot be proven}} \
                                // expected-warning {{self-comparison always evaluates to false}}
  _ContractAssert(x != 3);      // expected-error {{contract assertion cannot be proven}}
  if (x == 4)
    return 1;
  return x;
}
