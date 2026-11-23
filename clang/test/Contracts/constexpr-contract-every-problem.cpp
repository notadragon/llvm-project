// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -verify-ignore-unexpected=note %s

// During constant evaluation a false or not-constant predicate is recorded
// and evaluation continues.  If the evaluation completes, every recorded
// problem is reported at its own assertion, a not-constant one included.  If
// it does not complete, nothing recorded is reported and the only error is
// that the expression is not constant.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-every-problem.C

// Both preconditions are false and the evaluation completes.
constexpr int f1(const int &v)
  pre(false) // expected-error {{contract failed}}
  pre(false) // expected-error {{contract failed}}
{
  return 1;
}

int use1(int p) {
  const int k = f1(p); // expected-error {{initialization of constant-initialized variable failed}}
  return k;
}

// The first is not constant, the second false; the evaluation completes.
constexpr int f2(const int &v)
  pre(v > 0) // expected-error {{}}
  pre(false) // expected-error {{contract failed}}
{
  return 1;
}

int use2(int p) {
  const int k = f2(p); // expected-error {{initialization of constant-initialized variable failed}}
  return k;
}

// The body reads a non-constant object, so the constexpr variable is simply
// not constant; the false precondition is not reported.
int g = 1;

constexpr int f3(const int &v) pre(false) { return v; }

constexpr int k3 = f3(g); // expected-error {{must be initialized by a constant expression}}
