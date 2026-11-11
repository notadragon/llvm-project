// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": override shadows in templates, per
// instantiation of the overrider, with the overridden function's contract
// assertions as substituted for its own class -- a class template deriving
// from a dependent base, a non-template class deriving from a
// specialization, a class template deriving from a non-template class,
// and a destructor.  (A member function template cannot be virtual.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-template.C

template <class T>
struct TB {
  virtual T f(T x) pre(x > T(0)) post(r: r > T(0));  // expected-note 2 {{the predicate of the contract assertion}}
};

template <class T>
struct TD : TB<T> {
  T f(T x) override { return x; }
};

template <class T>
struct TE : TB<T> {
  T f(T x) override             // expected-error {{cannot be proven for a call through 'TB<int>::f'}}
  { return x - 1; }
};

struct ND : TB<int> {
  int f(int x) override         // expected-error {{cannot be proven for a call through 'TB<int>::f'}}
  { return -x; }
};

// The base's contract assertions depend on a non-type parameter.
template <int N>
struct NB {
  virtual int f(int x) pre(x > N) post(r: r > N);  // expected-note {{the predicate of the contract assertion}}
};

template <int N>
struct ND2 : NB<N> {
  int f(int x) override { return x; }
};

template <int N>
struct ND3 : NB<N> {
  int f(int x) override         // expected-error {{cannot be proven for a call through 'NB<3>::f'}}
  { return N; }
};

// A class template deriving from a non-template class.
struct B {
  virtual int f(int x) pre(x > 0) post(r: r > 0);
};

template <class T>
struct TF : B {
  int f(int x) override { return x + sizeof(T) - sizeof(T); }
};

// A destructor of a class template, implicitly defined.
int g;
template <class T>
struct DB {
  virtual ~DB() post(g == 1) { g = 1; }
};
template <class T>
struct DD : DB<T> { T t; };

TD<int> td;
TE<int> te;                     // expected-note {{in instantiation of member function 'TE<int>::f' requested here}}
TD<long> tdl;
ND nd;
ND2<3> nd2;
ND3<3> nd3;                     // expected-note {{in instantiation of member function 'ND3<3>::f' requested here}}
TF<char> tf;
DD<int> dd;
