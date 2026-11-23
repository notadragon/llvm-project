// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E150 and E152, at -O0: the
// override obligations of the overridden functions' postconditions and of
// destructors -- user-provided or implicitly defined -- in a virtual
// function this translation unit never compiles (an inline overrider whose
// class's vtable is emitted elsewhere, here because the key function is
// defined in another translation unit) are not diagnosed; that translation
// unit compiles the overrider and analyzes it.  Kept and used, the same
// class is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-uncompiled-post-O0.C

int g;

struct B {
  virtual int f(int x) post(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual ~B() post(g == 1);            // expected-note 2 {{the predicate of the contract assertion}}
};

struct Elsewhere : B {
  virtual void key();           // defined in another translation unit
  int f(int x) override
  { return x; }
  ~Elsewhere() override
  {}
};

struct Here : B {
  virtual void key();
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}}
  { return x; }
  ~Here() override              // expected-error {{cannot be proven for a call through 'B::~B'}}
  {}
};

void Here::key() {}

// An implicitly defined destructor, of an inline function this translation
// unit does not emit either.
struct ImplicitElsewhere : B {
  virtual void key();
};

inline void unused(ImplicitElsewhere *p) { p->~ImplicitElsewhere(); }

struct ImplicitHere : B {       // expected-error {{cannot be proven for a call through 'B::~B'}}
  virtual void key();
};

void ImplicitHere::key() {}
