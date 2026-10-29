// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": the overridden function's postconditions
// are proven at each return of the overrider -- not where it exits by an
// exception -- for a noexcept overrider, a throwing one, and one returning
// a class by the named return value.  None of these overriders has
// contract assertions of its own.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-eh.C

struct R { R(); R(const R &); ~R(); int v; };

struct B {
  virtual int f(int x) noexcept pre(x > 0) post(r: r > 0);
  virtual int g(int x) pre(x > 0) post(r: r > 0);
  virtual int h(int x) pre(x > 0) post(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual R make(int x) pre(x > 0) post(r: r.v > 0);
};

struct D : B {
  int f(int x) noexcept override { return x; }  // expected-remark {{override shadow: proven, conditional}}
  int g(int x) override         // expected-remark {{override shadow: proven, conditional}}
  {
    if (x > 100)
      throw 1;
    return x;
  }
  int h(int x) override         // expected-error {{cannot be proven for a call through 'B::h'}} expected-remark {{override shadow: not proven}}
  {
    if (x > 100)
      throw 1;
    return x - 1;
  }
  R make(int x) override        // expected-remark {{override shadow: proven, conditional}}
  {
    R r;
    r.v = x;
    return r;
  }
};

void use() { D d; }
