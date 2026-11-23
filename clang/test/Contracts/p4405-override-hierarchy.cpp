// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": every function a virtual function
// overrides, directly or indirectly, gets an override shadow of its own,
// which takes that function's preconditions as given and proves the
// overrider's at entry; `this' is the base class subobject.  Each
// namespace's use() constructs the classes, which emits their vtables and
// with them the overriders (DECISIONS.md E150).  The remarks give each
// override shadow's result, conditional where a premise from the overridden
// function's precondition is relied on (E151).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-hierarchy.C
// (gnu_gcc counts its override shadows in a dump; here each obligation's
// remark.  nocontracts::E::f's tautology, which gnu_gcc's folder proves, is
// proven in its override shadow: Clang's folding knows nothing of x)

// D's call through B evaluates B::f's and D::f's preconditions, never
// C::f's: D::f is analyzed against B::f and against C::f separately, and
// C::f's shadow against B::f does not cover D::f.
namespace multilevel {
  struct B { virtual void f(int x) pre(x > 0); };
  struct C : B {
    void f(int x) override      // expected-error {{cannot be proven for a call through 'multilevel::B::f'}} expected-remark {{override shadow: not proven}}
      pre(x > 5) {}             // expected-note {{the predicate of the contract assertion}}
  };
  struct D : C {
    void f(int x) override      // expected-error {{cannot be proven for a call through 'multilevel::B::f'}} expected-remark {{override shadow: not proven}} expected-remark {{override shadow: proven, conditional}}
      pre(x > 2) {}             // expected-note {{the predicate of the contract assertion}}
  };
  void use() { C c; D d; }
}

// Each base of a class with several: B is not D's primary base, so `this'
// is adjusted, and B::m is the same object in the premise and in D::f's
// precondition.
namespace multiple {
  struct A { virtual void f(int x) pre(x > 0); };
  struct B { int m; virtual void f(int x) pre(x > 10 && m > 0); };
  struct D : A, B {
    void f(int x) override      // expected-error {{cannot be proven for a call through 'multiple::A::f'}} expected-remark {{override shadow: not proven}} expected-remark {{override shadow: proven, conditional}}
      pre(x > 5 && m > 0) {}    // expected-note {{the predicate of the contract assertion}}
  };
  void use() { D d; }
}

// A virtual base, reached through two paths: one override shadow, with
// `this' adjusted through the vbase offset.
namespace vbase {
  struct V { int m; virtual int f() pre(m > 0); };
  struct L : virtual V {};
  struct R : virtual V {};
  struct D : L, R {
    int f() override pre(m > 0) { return m; }  // expected-remark {{override shadow: proven, conditional}}
  };
  struct E : L, R {
    int f() override            // expected-error {{cannot be proven for a call through 'vbase::V::f'}} expected-remark {{override shadow: not proven}}
      pre(m > 1) { return m; }  // expected-note {{the predicate of the contract assertion}}
  };
  void use() { D d; E e; }
}

// A pure virtual function's preconditions are still the premises.
namespace pure {
  struct B { virtual int f(int x) pre(x > 0) = 0; };
  struct D : B {
    int f(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, conditional}}
  };
  struct E : B {
    int f(int x) override       // expected-error {{cannot be proven for a call through 'pure::B::f'}} expected-remark {{override shadow: not proven}}
      pre(x > 1) { return x; }  // expected-note {{the predicate of the contract assertion}}
  };
  void use() { D d; E e; }
}

// An overridden function without contract assertions still gives an
// override shadow, with no premises: a call through B may pass any
// argument, so D::f's precondition must be a tautology (DECISIONS.md
// E156).  E::f's is one, proven soundly (nothing is relied on).
namespace nocontracts {
  struct B { virtual int f(int x); };
  struct D : B {
    int f(int x) override       // expected-error {{cannot be proven for a call through 'nocontracts::B::f'}} expected-remark {{override shadow: not proven}}
      pre(x > 100) { return x; }  // expected-note {{the predicate of the contract assertion}}
  };
  struct E : B {
    int f(int x) override pre(x > 100 || x <= 100) { return x; }  // expected-remark {{override shadow: proven, sound}}
  };
  void use() { D d; E e; }
}

// An overrider not defined in this translation unit has no body to analyze.
namespace declonly {
  struct B { virtual int f(int x) pre(x > 0); };
  struct D : B { int f(int x) override pre(x > 100); };
}
