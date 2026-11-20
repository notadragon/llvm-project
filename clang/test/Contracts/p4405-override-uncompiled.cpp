// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E150, at -O2: the
// override obligations of a virtual function this translation unit never
// compiles -- an inline overrider whose class's vtable is emitted
// elsewhere, here because the key function is defined in another
// translation unit -- are not diagnosed; that translation unit compiles the
// overrider and analyzes it.  Clang generates only the functions it emits,
// so it never analyzes them (E130's rule for ordinary evaluations of an
// unused inline function is the same).  Kept and used, the same class is
// diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-uncompiled.C

struct B { virtual int f(int x) pre(x > 0); };

struct Elsewhere : B {
  virtual void key();           // defined in another translation unit
  int f(int x) override
    pre(x > 10)
  { return x; }
};

struct Here : B {
  virtual void key();
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}}
    pre(x > 10)                 // expected-note {{the predicate of the contract assertion}}
  { return x; }
};

void Here::key() {}

// Decided at once (folding): diagnosed only if compiled as well.
struct ElsewhereFalse : B {
  virtual void key();
  int f(int x) override
    pre(false)
  { return x; }
};

struct HereFalse : B {
  virtual void key();
  int f(int x) override         // expected-error {{contract predicate is always false for a call through 'B::f'}}
    pre(false)                  // expected-note {{the predicate of the contract assertion}}
  { return x; }
};

void HereFalse::key() {}

// Constructing one needs the vtable, which the key function's translation
// unit emits: at -O2 Clang may emit it here available_externally, which
// emits no inline virtual function that is not used anyway.
B *make() { return new Elsewhere; }
