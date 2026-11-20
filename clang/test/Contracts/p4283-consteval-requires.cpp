// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify -verify-ignore-unexpected=note %s

// P4283 x consteval: a constrained contract assertion on a consteval
// function template (and on a contract_assert in one) is kept or discarded
// per instantiation, and a kept one is checked when the immediate
// invocation is evaluated.  int satisfies the constraint, so its false
// precondition makes the invocation ill-formed; double does not, so the
// same call is a constant.  A satisfied constraint with a true predicate is
// fine.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-consteval-requires.C

template <class T> concept Integral = __is_integral(T);

template <typename T>
consteval T f(T x)
  pre requires (Integral<T>) (x > 0) // expected-error 2 {{contract failed during execution of constexpr function}}
{
  return x;
}

template <typename T>
consteval T g(T x) {
  contract_assert requires (Integral<T>) (x > 0); // expected-error 2 {{contract failed during execution of constexpr function}}
  return x;
}

static_assert(f(1) == 1);
static_assert(f(-1.5) == -1.5); // discarded for double
static_assert(g(2) == 2);
static_assert(g(-2.5) == -2.5); // discarded for double
// Diagnosed at the assertions above (once for the variable's initializer
// and once for the immediate invocation, as for any failing consteval call).
constexpr int bad_f = f(-1); // expected-error {{constexpr variable 'bad_f' must be initialized by a constant expression}} \
                             // expected-error {{call to consteval function 'f<int>' is not a constant expression}}
constexpr int bad_g = g(-1); // expected-error {{constexpr variable 'bad_g' must be initialized by a constant expression}} \
                             // expected-error {{call to consteval function 'g<int>' is not a constant expression}}
