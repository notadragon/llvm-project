// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -O0 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405: a diagnostic of an evaluation in a template instantiation is
// followed by the notes Sema gives a diagnostic there ("in instantiation of
// ... requested here", each level), though code generation issues it once
// Sema has finished: Sema records them when it instantiates a function
// definition or a lambda's body, and code generation follows the
// diagnostic of an evaluation in that function with them (CLANG-830).
// Once for consecutive diagnostics of one instantiation, as Sema; none
// for a non-template function.  A function template called through
// another, an explicit instantiation, a member function of a class
// template, a lambda in a function template (with Sema's "while
// substituting into a lambda expression"), a generic lambda's
// specialization, and a local class's member function.  At -O0 the
// diagnostics are issued at once, at -O2 after the proof module.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-context.C
// (each prints its own compiler's backtrace)

int sink (int);

template <int N>
int f (int x)
{
  contract_assert (x > N);  // expected-warning 2 {{contract assertion cannot be proven}}
  contract_assert (N < 0);  // expected-warning 2 {{contract predicate is always false}}
  return sink (x);
}

template <int N>
int g (int x) { return f<N> (x); }  // expected-note {{in instantiation of function template specialization 'f<6>' requested here}}

int h (int x) { return g<6> (x); }  // expected-note {{in instantiation of function template specialization 'g<6>' requested here}}

template int f<7> (int);  // expected-note {{in instantiation of function template specialization 'f<7>' requested here}}

template <class T>
struct A
{
  int m (int x)
  {
    contract_assert (x > (int) sizeof (T));  // expected-warning {{contract assertion cannot be proven}}
    return sink (x);
  }
};

template <class T>
int ua (A<T> &a)
{
  return a.m (1);  // expected-note {{in instantiation of member function 'A<int>::m' requested here}}
}

int ub (A<int> &a) { return ua (a); }  // expected-note {{in instantiation of function template specialization 'ua<int>' requested here}}

template <int N>
int l (int x)
{
  auto L = [] (int y) {  // expected-note {{while substituting into a lambda expression here}}
    contract_assert (y > N);  // expected-warning {{contract assertion cannot be proven}}
    return sink (y);
  };
  return L (x);
}

int ul (int x) { return l<3> (x); }  // expected-note {{in instantiation of function template specialization 'l<3>' requested here}}

template <int N>
int gl (int x)
{
  auto L = [] (auto y) {
    contract_assert (y > N);  // expected-warning {{contract assertion cannot be proven}}
    return sink (y);
  };
  return L (x);  // expected-note {{in instantiation of function template specialization 'gl(int)::(lambda)::operator()<int>' requested here}}
}

int ugl (int x) { return gl<4> (x); }  // expected-note {{in instantiation of function template specialization 'gl<4>' requested here}}

template <int N>
int lc (int x)
{
  struct S  // expected-note {{in instantiation of member function 'lc(int)::S::m' requested here}}
  {
    int m (int y)
    {
      contract_assert (y > N);  // expected-warning {{contract assertion cannot be proven}}
      return sink (y);
    }
  };
  return S ().m (x);
}

int ulc (int x) { return lc<2> (x); }  // expected-note {{in instantiation of function template specialization 'lc<2>' requested here}}

int plain (int x)
{
  contract_assert (x > 0);  // expected-warning {{contract assertion cannot be proven}}
  return sink (x);
}
