// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s -fcontracts
// XFAIL: *

// Two lambda-specific constification shapes Clang gets wrong, in opposite
// directions.  GCC is right about both.  Found by writing the mirror for
// gnu_gcc 5442adee87a, not by looking for them.
//
// 1. A member reached through a CAPTURED `*this` is over-constified.
//    [expr.prim.id.unqual]/3+e marks `++this->z` and `++z` "OK, captured
//    *this" -- the rule constifies a *variable* declared outside the contract
//    assertion, and a non-static data member of the captured object is not
//    one.  Clang reports "cannot assign to non-static data member because it
//    is considered 'const' inside of a contract".
//
// 2. A FUNCTION-LOCAL STATIC is under-constified when named inside a lambda in
//    the predicate.  It is "a variable declared outside of C" like any other,
//    and Clang constifies it correctly when it is named DIRECTLY in the
//    predicate -- see the control in the companion file.  Only the lambda case
//    is missed, which is what makes this a lambda bug rather than a
//    storage-duration one.
//
// The two are almost certainly not one fix: (1) is the member-access path
// deciding it is inside a contract when the object is the closure's captured
// copy, and (2) is getContractConstification's "declared outside of C" test
// not reaching a local static through the intervening lambda scopes.
//
// GCC mirror: g++.dg/contracts/cpp26/contract-predicate-constify-lambda.C,
// where both shapes are asserted the way the paper states them.

// expected-no-diagnostics

struct Y {
  int z = 0;
  // Must be accepted: the closure captured *this, so these name members of
  // the captured copy, not a variable declared outside the predicate.
  void f() pre([*this]() mutable {
    ++this->z;
    ++z;
    return true;
  }()) {}
};

void local_static_in_lambda() {
  static int s = 0;
  // Must be REJECTED -- the directive above says the file is diagnostic-free
  // only because it is XFAILed as a whole; when this is fixed, this line wants
  // an expected-error and the file's expected-no-diagnostics goes away along
  // with the XFAIL.
  contract_assert([&]() { ++s; return true; }());
}
