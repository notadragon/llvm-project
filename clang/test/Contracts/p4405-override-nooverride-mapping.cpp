// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): B::f's
// postconditions, obligations of D::f's own proven in its proof shadow, see
// what a call through B::f sees -- `this' the B subobject (here not the
// primary base, so adjusted), the result converted to B::f's return type
// for a covariant one, a reference parameter the object it refers to, and a
// by-value parameter as it was at entry, since the P3097 wrapper checks the
// postcondition with its own copy, which D::f cannot modify (DECISIONS.md
// E153).  (Clang's own test; gnu_gcc's p4405-override-covariant.C and
// p4405-override-param.C test the same mapping in override shadows.)

struct Pad { int pad[4]; virtual void p(); };

struct B {
  int m;
  virtual B *self() post(r: r == this);   // expected-note {{the predicate of the contract assertion}}
  virtual int get(const int x) post(r: r == x);  // expected-note {{the predicate of the contract assertion}}
  virtual int member() post(r: r == m);
  virtual int &ref(int &x) post(r: &r == &x);
};

struct D : Pad, B {
  D *self() override { return this; }                      // expected-remark {{proof shadow: proven, sound}}
  int get(int x) override { int r = x; x = 0; return r; }  // expected-remark {{proof shadow: proven, sound}}
  int member() override { return m; }                      // expected-remark {{proof shadow: proven, sound}}
  int &ref(int &x) override { return x; }                  // expected-remark {{proof shadow: proven, sound}}
};

struct E;
E *other();

struct E : Pad, B {
  E *self() override { return other(); }   // expected-error {{cannot be proven for a call through 'B::self'}} expected-remark {{proof shadow: not proven}}
  int get(int x) override { x = 0; return x; }  // expected-error {{cannot be proven for a call through 'B::get'}} expected-remark {{proof shadow: not proven}}
};

void use() { D d; E e; }
