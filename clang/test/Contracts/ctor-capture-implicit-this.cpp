// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

// The implicit (*this) transformation is ill-formed in the initializer of a
// postcondition capture of a constructor (P3098 [expr.prim.id.general]), as
// in a constructor precondition; in a destructor's capture, or through an
// explicit this, or unevaluated, it is fine.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/cdtor-capture-implicit-this-ctor.C

struct A1 {
  int v = 0;
  A1() post [c = v] (c == 0) {} // expected-error {{cannot access non-static member 'v' in the postcondition capture initializer of a constructor}}
};
struct A2 {
  int v = 0;
  ~A2() post [c = v] (c == 0) {} // OK
};
struct A3 {
  int v = 0;
  A3() post [c = this->v] (c == 0) {}          // OK
  A3(int) post [c = sizeof(v)] (c > 0) {}      // OK, unevaluated
};
