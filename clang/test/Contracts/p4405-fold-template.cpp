// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405 stage 2: folding happens per template instantiation, after
// substitution, so one instantiation can be proven and another always false
// or unproven.  An uninstantiated template is not diagnosed.  Each
// instantiation's diagnostics are followed by its note, once (CLANG-830).
// A precondition is analyzed at each call (D4405), with the argument: the
// call's own value decides x == x (a double may be a NaN).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-template.C

template <int N>
void f() {
  contract_assert(N > 0);       // expected-error {{contract predicate is always false}}
}

template <class T>
T g(T x)
  pre(sizeof(T) >= 4)           // expected-note {{the predicate of the contract assertion}}
  pre(x == x)                   // expected-note {{the predicate of the contract assertion}}
{
  return x;
}

template <class T>
void never_used(T) {
  contract_assert(false);
}

void use(double d) {
  f<1>();                       // proven
  f<0>();                       // the "always false" above; expected-note {{in instantiation of function template specialization 'f<0>' requested here}}
  g(1);                         // both proven
  g('c');                       // expected-error {{contract predicate is always false}}
  g(1.0);                       // proven: 1.0 == 1.0
  g(d);                         // expected-error {{contract assertion cannot be proven}}
}
