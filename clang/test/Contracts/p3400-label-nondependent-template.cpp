// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only -Xclang -verify -Xclang -fexperimental-new-constant-interpreter %s
// REQUIRES: contracts-libcxx

// P3400: a label that does not depend on the template parameters, on a
// contract assertion of a function template, is resolved where the
// template is declared, and its allowed_semantics facet is a constant
// expression there too.  gnu_gcc rejected it as "must be usable in a
// constant expression" (its GCC-916); Clang did not.  Each label restricts
// the semantics as it does on a function that is not a template: constant
// evaluation with the configured observe finds the predicates false (Clang:
// "contract failed during execution of constexpr function", and the
// initializer is not a constant expression).  Both constant evaluators.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-nondependent-template.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct my_enforce_t {
  using assertion_control_object = my_enforce_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::enforce};
};
constexpr my_enforce_t my_enforce{};

template <typename T>
constexpr T e(T x) pre<my_enforce>(x > 0) { return x; }  // expected-error {{contract failed during execution of constexpr function}}

template <typename T>
constexpr T t(T x) pre<std::contracts::labels::terminating>(x > 0)  // expected-error {{contract failed during execution of constexpr function}}
{
  contract_assert<my_enforce>(x != 2);  // expected-error {{contract failed during execution of constexpr function}}
  return x;
}

template <typename T>
T d(T x) pre<my_enforce>(x > 0);

constexpr int a = e(1);
constexpr int b = e(-1);  // expected-error {{constexpr variable 'b' must be initialized by a constant expression}} expected-note {{in call to 'e<int>(-1)'}}
constexpr int c = t(2);  // expected-error {{constexpr variable 'c' must be initialized by a constant expression}} expected-note {{in call to 't<int>(2)'}}
constexpr int f = t(-1);  // expected-error {{constexpr variable 'f' must be initialized by a constant expression}} expected-note {{in call to 't<int>(-1)'}}
int g(int x) { return d(x); }
