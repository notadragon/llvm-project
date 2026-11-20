// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify -verify-ignore-unexpected=note %s

// A not-constant precondition met while deciding whether a const local is
// constant-initialized, under observe.  The evaluation completes, so the
// predicate is a contract violation reported as a warning, and the variable
// is still constant-initialized (usable in a constant expression).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-observe-not-constant-init.C,
// an expected pass there too.

constexpr int f(const int &v)
  pre(v > 0) // expected-warning {{}}
{
  return 1;
}

int use(int p) {
  const int k = f(p);
  static_assert(k == 1);
  return k;
}
