// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s
// expected-no-diagnostics

// Under the ignore semantic a contract predicate is not evaluated during
// constant evaluation, so one that would exit via an exception -- a throw
// written in the predicate, or a call that throws -- is not diagnosed and the
// enclosing expression stays constant.
//
// Mirror: g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate-ignore.C

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
  contract_assert((throw 1, true));
  ++n;
  contract_assert(thr());
  ++n;
  contract_assert((throw owner(), true));
  ++n;
  return n;
}

static_assert(f() == 3);

constexpr int pre_throw(int x)
  pre((throw x, true))
{
  return x;
}

constexpr int post_throw(const int x)
  post((throw x, true))
{
  return x;
}

static_assert(pre_throw(1) == 1);
static_assert(post_throw(2) == 2);
