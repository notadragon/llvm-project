// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -fsyntax-only \
// RUN:   -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D3400R5 [support.contract.control.compute] and [basic.contract.control]
// make a type a computed-semantics control type only if t.compute_semantic(s)
// is implicitly convertible to evaluation_semantic (convertible_to;
// CLANG-828).  Both the library concept and Sema's compute_semantic facet
// follow that: a facet returning int, another enumeration, or a class with
// only an explicit conversion does not participate, and one returning a class
// whose conversion is implicit does, through that conversion.
// Runnable/p3595-dynamic-compute-convertible.cpp is the dynamic-path
// counterpart.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-convertible.C

#include <contracts>

using std::contracts::evaluation_semantic;
namespace L = std::contracts::labels;

// Not computed-semantics control types: the result converts to
// evaluation_semantic only explicitly.
struct int_t {
  using assertion_control_object = int_t;
  constexpr int compute_semantic(evaluation_semantic) const {
    return (int)evaluation_semantic::ignore;
  }
};
constexpr int_t int_l{};

enum class other : int { ignore_like = (int)evaluation_semantic::ignore };
struct enum_t {
  using assertion_control_object = enum_t;
  constexpr other compute_semantic(evaluation_semantic) const {
    return other::ignore_like;
  }
};
constexpr enum_t enum_l{};

struct explicit_sem {
  constexpr explicit operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct explicit_t {
  using assertion_control_object = explicit_t;
  constexpr explicit_sem compute_semantic(evaluation_semantic) const {
    return {};
  }
};
constexpr explicit_t explicit_l{};

// A computed-semantics control type: the result converts implicitly.
struct implicit_sem {
  constexpr operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct implicit_t {
  using assertion_control_object = implicit_t;
  constexpr implicit_sem compute_semantic(evaluation_semantic) const {
    return {};
  }
};
constexpr implicit_t implicit_l{};

// The library concept.
static_assert(L::semantic_computation_label<implicit_t>);
static_assert(!L::semantic_computation_label<int_t>);
static_assert(!L::semantic_computation_label<enum_t>);
static_assert(!L::semantic_computation_label<explicit_t>);

// Sema.  Each facet maps enforce to ignore; where the label is not a
// computed-semantics control type the assertion stays enforce, and its false
// predicate fails the constant evaluation.
constexpr int f_int(int x) {
  contract_assert<int_l>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_int(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{in call to 'f_int(-1)'}}

constexpr int f_enum(int x) {
  contract_assert<enum_l>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_enum(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{in call to 'f_enum(-1)'}}

constexpr int f_explicit(int x) {
  contract_assert<explicit_l>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_explicit(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note {{in call to 'f_explicit(-1)'}}

constexpr int f_implicit(int x) {
  contract_assert<implicit_l>(x > 0);
  return 1;
}
static_assert(f_implicit(-1));
