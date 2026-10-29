// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": with a covariant return type, the
// overridden function's postconditions see the overrider's result
// converted to the overridden function's return type, as the covariant
// thunk converts it -- null-checked for a pointer -- and `this' is the
// base class subobject.  B is not D's primary base, so both conversions
// adjust the pointer.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-covariant.C

struct X { int pad; virtual void x(); };

struct B {
  virtual B *self() post(r: r == this);
  virtual B &ref() post(r: &r == this);
  virtual B *maybe(bool b) post(r: r != nullptr);  // expected-note {{the predicate of the contract assertion}}
  virtual B *never() post(r: r == nullptr);
};

struct D : X, B {
  D *self() override { return this; }  // expected-remark {{override shadow: proven, sound}}
  D &ref() override { return *this; }  // expected-remark {{override shadow: proven, sound}}
  // Not proven: a null result stays null in the conversion.
  D *maybe(bool b) override            // expected-error {{cannot be proven for a call through 'B::maybe'}} expected-remark {{override shadow: not proven}}
  { return b ? this : nullptr; }
  D *never() override { return nullptr; }  // expected-remark {{override shadow: proven, sound}}
};

void use() { D d; }
