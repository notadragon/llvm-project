// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3, -fcontracts-prove-shadow (implied by -fcontracts-p4405):
// at -O2 a prove evaluation is proven when the optimizers can show, in the
// function's proof shadow, that its predicate cannot be false where it is
// evaluated -- a dominating condition, a value propagated through an
// inlined helper, the function's own arithmetic -- and diagnosed otherwise.
// P4021R2 Example 2 (an index reaching the array size) stays ill-formed --
// and is not "always false", since it holds for the other iterations.
// Preconditions (at a call), postconditions and assertions.  A predicate
// the shadow shows false whenever it is evaluated is "always false"
// (Clang's own case: folding cannot read a local variable).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-prove.C (GCC
// also warns -Waggressive-loop-optimizations about Example 2's loop)

static int clamp(int v) { return v < 0 ? 0 : v > 9 ? 9 : v; }

int guarded(const int *a, int i, int n)
  pre(n >= 0)                   // expected-note {{the predicate of the contract assertion}}
{
  if (i >= 0 && i < n) {
    contract_assert(i < n);
    contract_assert(i >= 0);
    return a[i];
  }
  contract_assert(i < n);       // expected-error {{contract assertion cannot be proven}}
  return 0;
}

int call_guarded(const int *a, int i, int n)
{
  int r = guarded(a, i, 4);
  if (n > 0)
    r += guarded(a, i, n);
  return r + guarded(a, i, n);  // expected-error {{contract assertion cannot be proven}}
}

int propagated(int v) {
  int c = clamp(v);
  contract_assert(c >= 0 && c <= 9);
  contract_assert(c < 9);       // expected-error {{contract assertion cannot be proven}}
  return c;
}

int result(int x)
  post(r: r > 0)
  post(r: r > 1)                // expected-error {{contract assertion cannot be proven}}
{
  return x > 0 ? x : 1;
}

// P4021R2 Example 2.
int example2() {
  const int buf_size = 4;
  char buf[buf_size];
  for (int i = 0; i != 5; ++i) {
    contract_assert(i < buf_size);  // expected-error {{contract assertion cannot be proven}}
    buf[i] = 3;
  }
  return buf[0];
}

int bounded() {
  const int buf_size = 4;
  char buf[buf_size];
  for (int i = 0; i != 4; ++i) {
    contract_assert(i < buf_size);
    buf[i] = 4;
  }
  return buf[0];
}

int always_false(int y) {
  int k = 5;
  contract_assert(k != 5);      // expected-error {{contract predicate is always false}}
  return k + y;
}
