// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function"): converting a closure to a
// pointer to function takes the address of its function call operator -- a
// call through the pointer, or through the static invoker it may point to,
// has no analysis evaluation.  So a precondition of the operator that
// allows no runtime semantic, and that folding does not prove, cannot be
// proven for a call through the address: an error under prove
// (always_prove) and a -Wcontract-unproven warning under diagnose
// (always_diagnose), for a lambda, a C++23 static lambda (whose conversion
// returns the operator itself) and a generic lambda (the specialization
// converted to).  A tautological one is not diagnosed.  The conversion
// functions themselves and the invoker's call of the operator, whose
// arguments are unknown, are not analyzed.  Neither is an unevaluated
// conversion.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-lambda-address.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

auto unused = [](int x) pre<always_prove>(x > 0) { return x; };
auto unused_static = [](int x) static pre<always_prove>(x > 0) { return x; };

int use(int y) {
  auto l1 = [](int x) pre<always_prove>(x > 0) { return x; };   // expected-note {{precondition here}}
  int (*p1)(int) = l1; // expected-error {{contract assertion cannot be proven for a call through the address of 'operator()'}}
  auto l2 = [](int x) static pre<always_prove>(x > 0) { return x; }; // expected-note {{precondition here}}
  int (*p2)(int) = l2; // expected-error {{cannot be proven for a call through the address}}
  auto l3 = [](auto x) pre<always_prove>(x > 0) { return x; };  // expected-note {{precondition here}}
  int (*p3)(int) = l3; // expected-error {{cannot be proven for a call through the address}}
  auto l4 = [](int x) pre<always_diagnose>(x > 0) { return x; }; // expected-note 2 {{precondition here}}
  int (*p4)(int) = +l4; // expected-warning {{contract assertion cannot be proven for a call through the address of 'operator()'}}
  auto p5 = static_cast<int (*)(int)>(l4); // expected-warning {{cannot be proven for a call through the address}}
  auto l6 = [](auto x) static pre<always_diagnose>(x > 0) { return x; }; // expected-note {{precondition here}}
  long (*p6)(long) = l6; // expected-warning {{cannot be proven for a call through the address}}
  auto l7 = [](int x) pre(x > 0) { return x; };
  int (*p7)(int) = l7;
  auto l8 = [](int x) pre<always_prove>(sizeof(x) == sizeof(int)) { return x; };
  int (*p8)(int) = l8;
  decltype(+l1) q = nullptr;
  (void)sizeof(+l2);
  int r = l1(1) + l2(2) + l3(3) + l4(4) + l6(6);
  return r + p1(y) + p2(y) + p3(y) + p4(y) + p5(y) + p6(y) + p7(y) +
         p8(y) + (q ? 1 : 0);
}

// In a template, at instantiation.
template <typename T> T in_template(T y) {
  auto l = [](T x) pre<always_prove>(x > 0) { return x; }; // expected-note {{precondition here}}
  T (*p)(T) = l; // expected-error {{cannot be proven for a call through the address}}
  return p(y);
}

template int in_template<int>(int); // expected-note {{in instantiation of function template specialization 'in_template<int>' requested here}}
