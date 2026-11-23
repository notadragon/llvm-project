// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -clear-ast-before-backend -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=prove -O2 -fno-contracts-prove-shadow -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a diagnostic, issued at once or by the proof shadows, of
// an evaluation outside any template instantiation has no instantiation
// notes: in a function, in a lambda's call operator, in a member function
// of a class, also right after one that has them.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-context-none.C
// (GCC's has no lambda, member function or template)

int helper (int x)
{
  contract_assert (x > 0);  // expected-error {{contract assertion cannot be proven}}
  return x;
}

int plain (int x)
{
  contract_assert (x > 1);  // expected-error {{contract assertion cannot be proven}}
  return helper (x);
}

int lam (int x)
{
  auto L = [] (int y) {
    contract_assert (y > 2);  // expected-error {{contract assertion cannot be proven}}
    return y;
  };
  return L (x);
}

struct S
{
  int m (int x)
  {
    contract_assert (x > 3);  // expected-error {{contract assertion cannot be proven}}
    return x;
  }
};

int use_s (S &s, int x) { return s.m (x); }

template <int N>
int t (int x)
{
  contract_assert (x > N);  // expected-error {{contract assertion cannot be proven}}
  return x;
}

int use_t (int x) { return t<4> (x); }  // expected-note {{in instantiation of function template specialization 't<4>' requested here}}

int after (int x)
{
  contract_assert (x > 5);  // expected-error {{contract assertion cannot be proven}}
  return x;
}
