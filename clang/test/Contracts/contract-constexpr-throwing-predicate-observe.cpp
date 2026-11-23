// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=observe -fsyntax-only -verify %s

// A contract predicate that exits via an exception during constant
// evaluation is a contract violation ([basic.contract.eval]); with the
// observe semantic it is diagnosed and the evaluation continues after the
// assertion, so the enclosing expression stays constant.  Both shapes: a
// throw written in the predicate, and a call that throws.  Clang does not
// implement P3068, so the exception object (here, one owning heap storage) is
// never created and nothing is reported as never deallocated.
//
// Mirror: g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate-observe.C

constexpr bool thr() {
  throw 1;
  return true;
}

// Owns heap storage, freed only by its destructor.
struct owner {
  int *p;
  constexpr owner() : p(new int(1)) {}
  constexpr owner(const owner &o) : p(new int(*o.p)) {}
  constexpr ~owner() { delete p; }
};

constexpr int f() {
  int n = 0;
  contract_assert((throw 1, true)); // expected-warning {{contract failed during execution of constexpr function}}
  ++n;
  contract_assert(thr()); // expected-warning {{contract failed during execution of constexpr function}}
  ++n;
  contract_assert((throw owner(), true)); // expected-warning {{contract failed during execution of constexpr function}}
  ++n;
  return n;
}

static_assert(f() == 3);

constexpr int pre_throw(int x)
  pre((throw x, true)) // expected-warning {{contract failed during execution of constexpr function}}
{
  return x;
}

constexpr int post_throw(const int x)
  post((throw x, true)) // expected-warning {{contract failed during execution of constexpr function}}
{
  return x;
}

static_assert(pre_throw(1) == 1);
static_assert(post_throw(2) == 2);
