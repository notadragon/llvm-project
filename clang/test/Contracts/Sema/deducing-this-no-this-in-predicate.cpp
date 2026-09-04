// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s -fcontracts
// XFAIL: *

// `this` is unavailable inside an explicit object member function
// ([expr.prim.this]/1), and a contract predicate on such a function is inside
// it.  Naming `this` -- explicitly, or implicitly by naming a non-static data
// member unqualified, which means (*this).m -- must be rejected.
//
// Clang accepts all of it, and then CodeGen asserts:
//
//   CodeGenFunction.h: LoadCXXThis():
//   Assertion `CXXThisValue && "no 'this' value for this function"' failed.
//
// The control at the bottom is what makes this contracts-specific rather than
// a general deducing-this hole: the identical use of `this` in the function
// BODY is correctly rejected, with "invalid use of 'this' in a function with
// an explicit object parameter".  Only the predicate is unguarded.
//
// -fsyntax-only, as here, does not reach the assertion -- it is a CodeGen
// failure -- so this file XFAILs on the missing diagnostics, and a compile
// of the same shapes crashes.  GCC rejects every case below.
//
// GCC mirror: g++.dg/contracts/cpp26/deducing-this-no-this-in-predicate.C

struct S {
  int x = 0;
};

// Explicit `this` in a precondition.
struct ExplicitThis : S {
  // expected-error@+1 {{invalid use of 'this' in a function with an explicit object parameter}}
  void f(this ExplicitThis &self) pre(this->x == 0);
};

// An unqualified non-static data member in a precondition.
struct ImplicitThisPre : S {
  // expected-error@+1 {{invalid use of 'this' in a function with an explicit object parameter}}
  void f(this ImplicitThisPre &self) pre(x == 0);
};

// The same in a postcondition.
struct ImplicitThisPost : S {
  // expected-error@+1 {{invalid use of 'this' in a function with an explicit object parameter}}
  int f(this ImplicitThisPost &self) post(r : x == r);
};

// The same in an assertion-statement in the body.
struct ImplicitThisAssert : S {
  void f(this ImplicitThisAssert &self) {
    // expected-error@+1 {{invalid use of 'this' in a function with an explicit object parameter}}
    contract_assert(x == 0);
  }
};

// CONTROL: the explicit object parameter itself is of course usable.
struct ViaSelf : S {
  void f(this ViaSelf &self) pre(self.x == 0);
};

// CONTROL: an ordinary (implicit object) member function may name `this` and
// its members in a predicate.
struct ImplicitObject : S {
  void g() const pre(this->x == 0) pre(x == 0);
};

// CONTROL: the same use of `this` in the BODY is already rejected.  This one
// passes today; it is here so that a future fix cannot be mistaken for having
// merely disabled the body check.
struct ThisInBody : S {
  void f(this ThisInBody &self) {
    (void)self;
    // expected-error@+1 {{invalid use of 'this' in a function with an explicit object parameter}}
    (void)this->x;
  }
};
