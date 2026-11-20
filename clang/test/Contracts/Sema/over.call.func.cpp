// [over.call.func]/p3.1 and [expr.prim.id.general]: an unqualified reference to a
// non-static member (which forms an implied 'this' access) in a constructor
// precondition or a destructor postcondition is ill-formed, because no object is
// within its lifetime at that point. Qualified access (this->m), constructor
// postconditions, and ordinary member functions are unaffected.
// (GCC mirror: g++.dg/contracts/cpp26/over.call.func.p3.1.C.)
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

struct S {
  bool f();
  bool k;

  S() pre(k); // expected-error {{cannot access non-static member 'k' in the precondition of a constructor because no object is within its lifetime there}}
  S(int) pre(this->k);     // OK: qualified access is not an unqualified member reference
  S(long) post(k);         // OK: constructor postcondition -- object is alive
  ~S() post(f());          // expected-error {{cannot access non-static member 'f' in the postcondition of a destructor because no object is within its lifetime there}}
  void g() pre(k) post(k); // OK: ordinary member function
};

// A destructor precondition and a constructor postcondition are fine (object
// within its lifetime); only ctor-pre / dtor-post are restricted.
struct T {
  bool f() const; // const so the constified 'this' in a contract can call it
  bool k;
  ~T() pre(f());   // OK: destructor precondition
  T() post(k);     // OK: constructor postcondition
};
