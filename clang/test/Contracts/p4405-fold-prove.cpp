// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -fexperimental-new-constant-interpreter -DNEW_INTERP -verify %s

// P4405 stage 2, -fcontracts-prove-fold (implied by -fcontracts-p4405): an
// evaluation with the prove semantic whose predicate folds to true is
// proven and not diagnosed, whether or not the predicate is a constant
// expression; one that folds to false is "always false"; anything else
// still "cannot be proven".  Preconditions (at each call, D4405),
// postconditions and assertions.  Both constant evaluators (h() with the
// classic one only).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-prove.C.  g()
// holds what GCC folds and Clang does not: Clang folds with the constant
// evaluator, which knows nothing of a parameter's value, so each is
// unproven here.  h() holds Clang's own cases.

constexpr int N = 3;
constexpr bool positive(int v) { return v > 0; }

int f(int x)
  pre(true)
  pre(sizeof(int) >= 1)
  pre(x > 0)                            // expected-note {{the predicate of the contract assertion}}
  post(r: N > 0)
  post(r: r > 0)                        // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(1 + 1 == 2);
  contract_assert(positive(N));
  contract_assert(false);               // expected-error {{contract predicate is always false}}
  contract_assert(sizeof(int) == 0);    // expected-error {{contract predicate is always false}}
  contract_assert(positive(-N));        // expected-error {{contract predicate is always false}}
  contract_assert(x != 3);              // expected-error {{contract assertion cannot be proven}}
  return x;
}

// GCC folds these, though none is a constant expression; Clang does not
// (though Sema warns that a self-comparison is always true or false).
int g(int x, unsigned u, int *p)
  pre(x == x)                           // expected-note {{the predicate of the contract assertion}} \
                                        // expected-warning {{self-comparison always evaluates to true}}
  pre(u >= 0u)                          // expected-note {{the predicate of the contract assertion}}
  pre(x != x)                           // expected-note {{the predicate of the contract assertion}} \
                                        // expected-warning {{self-comparison always evaluates to false}}
  pre(u < 0u)                           // expected-note {{the predicate of the contract assertion}}
  post(r: r == r)                       // expected-error {{contract assertion cannot be proven}} \
                                        // expected-warning {{self-comparison always evaluates to true}}
{
  contract_assert(p == p);              // expected-error {{contract assertion cannot be proven}} \
                                        // expected-warning {{self-comparison always evaluates to true}}
  contract_assert(x * 0 == 0);          // expected-error {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls(int y, unsigned v, int *q) {
  f(y); // expected-error {{contract assertion cannot be proven}}
  g(y, v, q); // expected-error 4 {{contract assertion cannot be proven}}
}

// Clang's own: an operand that cannot be folded does not matter when the
// rest decides the value and the predicate has no side effects; a constant
// local is folded; folding is not manifestly constant-evaluated, so
// __builtin_is_constant_evaluated() is false, as at run time; and
// __builtin_constant_p of a parameter is not folded at all (the optimizer
// may yet find it constant).
#ifndef NEW_INTERP
int h(int x)
  pre(x > 0 || true)
  pre(x > 0 && false)                   // expected-note {{the predicate of the contract assertion}}
  pre(!__builtin_is_constant_evaluated())
  pre(__builtin_constant_p(x) || x > 0) // expected-note {{the predicate of the contract assertion}}
{
  const int k = 4;
  contract_assert(k == 4);
  contract_assert(k + x > 0);           // expected-error {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls_h(int y) {
  h(y); // expected-error {{contract predicate is always false}} \
        // expected-error {{contract assertion cannot be proven}}
}
#endif
