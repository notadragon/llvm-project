// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", the return side through several levels
// and several bases: each overridden function's postconditions are proven
// in that function's own override shadow, and `this' in them is its base
// class subobject.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-hierarchy-post.C
// (gnu_gcc counts its override shadows in a dump; here each obligation's
// remark)

// D's call through B evaluates B::f's postconditions and through C, C::f's:
// E::f satisfies B::f's but not C::f's.
namespace multilevel {
  struct B { virtual int f(int x) post(r: r > 0); };
  struct C : B {
    int f(int x) override       // expected-remark 2 {{override shadow: proven, sound}}
      post(r: r > 1)            // expected-remark {{proof shadow: proven, sound}} expected-note {{the predicate of the contract assertion}}
    { return 5; }
  };
  struct D : C {
    int f(int x) override { return 2; }  // expected-remark 2 {{override shadow: proven, sound}}
  };
  struct E : C {
    int f(int x) override       // expected-error {{contract predicate is always false for a call through 'multilevel::C::f'}} expected-remark {{override shadow: always false}} expected-remark {{override shadow: proven, sound}}
    { return 1; }
  };
  void use() { C c; D d; E e; }
}

// B is not D's primary base: B::f's `m' is read through D's `this'
// adjusted to the B subobject, the same object D::f's body writes.
namespace multiple {
  struct A { virtual int f(int x) post(r: r > 0); };  // expected-note {{the predicate of the contract assertion}}
  struct B { int m; virtual int f(int x) post(r: r == m); };
  struct D : A, B {
    int f(int x) override { m = 7; return 7; }  // expected-remark 2 {{override shadow: proven, sound}}
  };
  struct E : A, B {
    int f(int x) override       // expected-error {{contract predicate is always false for a call through 'multiple::A::f'}} expected-remark {{override shadow: always false}} expected-remark {{override shadow: proven, sound}}
    { m = 0; return 0; }
  };
  void use() { D d; E e; }
}
