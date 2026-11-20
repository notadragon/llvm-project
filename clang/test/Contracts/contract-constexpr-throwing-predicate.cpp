// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=enforce -verify-ignore-unexpected=note -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=quick_enforce -verify-ignore-unexpected=note -fsyntax-only -verify %s

// A contract predicate that exits via an exception during constant
// evaluation is a contract violation ([basic.contract.eval]); with a
// terminating semantic the enclosing manifestly constant-evaluated
// expression is ill-formed.  Both shapes: a throw written in the predicate,
// and a call that throws.  Clang does not implement P3068, so a throw is
// never a core constant expression and the predicate is diagnosed as one
// that is not; the exception object (here, one owning heap storage) is never
// created, so nothing is reported as never deallocated.
//
// Mirror: g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate.C,
// g++.dg/contracts/cpp26/contract-constexpr-throwing-predicate-quick-enforce.C

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

constexpr bool inline_throw() {
  contract_assert((throw 1, true)); // expected-error {{contract failed during execution of constexpr function}}
  return true;
}

constexpr bool throwing_call() {
  contract_assert(thr()); // expected-error {{contract failed during execution of constexpr function}}
  return true;
}

constexpr bool owning_exception() {
  contract_assert((throw owner(), true)); // expected-error {{contract failed during execution of constexpr function}}
  return true;
}

constexpr bool pre_throw(int x)
  pre((throw x, true)) // expected-error {{contract failed during execution of constexpr function}}
{
  return true;
}

constexpr bool post_throw(const int x)
  post((throw x, true)) // expected-error {{contract failed during execution of constexpr function}}
{
  return true;
}

static_assert(inline_throw());     // expected-error {{static assertion expression is not an integral constant expression}}
static_assert(throwing_call());    // expected-error {{static assertion expression is not an integral constant expression}}
static_assert(owning_exception()); // expected-error {{static assertion expression is not an integral constant expression}}
static_assert(pre_throw(1));       // expected-error {{static assertion expression is not an integral constant expression}}
static_assert(post_throw(1));      // expected-error {{static assertion expression is not an integral constant expression}}

// CONTROL: an exception the predicate catches itself does not exit it, so
// with P3068 the predicate is true.  Without it the throw is not a core
// constant expression, which is itself a violation.
constexpr bool caught_inside() {
  contract_assert([] { try { throw 1; } catch (int) { return true; } }());
#if !defined(__cpp_constexpr_exceptions)
  // expected-error@-2 {{contract failed during execution of constexpr function}}
#endif
  return true;
}

static_assert(caught_inside());
#if !defined(__cpp_constexpr_exceptions)
// expected-error@-2 {{static assertion expression is not an integral constant expression}}
#endif

// Not manifestly constant-evaluated: no diagnostic, the violation is left to
// run time.
bool at_run_time() { return inline_throw() && throwing_call(); }
