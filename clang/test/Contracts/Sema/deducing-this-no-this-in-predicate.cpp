// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s -fcontracts

// `this` is unavailable inside an explicit object member function
// ([expr.prim.this]/1), and a contract predicate on such a function is inside
// it.  Naming `this` -- explicitly, or implicitly by naming a non-static data
// member unqualified, which means (*this).m -- must be rejected.
//
// All of it used to be accepted, and CodeGen then asserted:
//
//   CodeGenFunction.h: LoadCXXThis():
//   Assertion `CXXThisValue && "no 'this' value for this function"' failed.
//
// Sema::CheckCXXThisType already handled the explicit-object case correctly,
// but it only fires when the `this` type is null, and parsing a contract
// pushed a CXXThisScopeRAII for any member declarator -- the shared helper's
// notion of "member function" predates P0847 -- so that branch was
// unreachable.  The contract parser no longer pushes the scope for an explicit
// object member function.
//
// The control at the bottom is what made this contracts-specific rather than a
// general deducing-this hole: the identical use of `this` in the function BODY
// was rejected correctly all along.  Only the predicate was unguarded.
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
  // expected-error@+1 {{invalid use of member 'x' in explicit object member function}}
  void f(this ImplicitThisPre &self) pre(x == 0);
};

// The same in a postcondition.
struct ImplicitThisPost : S {
  // expected-error@+1 {{invalid use of member 'x' in explicit object member function}}
  int f(this ImplicitThisPost &self) post(r : x == r);
};

// The same in an assertion-statement in the body.
struct ImplicitThisAssert : S {
  void f(this ImplicitThisAssert &self) {
    // expected-error@+1 {{invalid use of member 'x' in explicit object member function}}
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
