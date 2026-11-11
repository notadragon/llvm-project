// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fcontracts-p3400 -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s

// P4283 in constant evaluation: an instance that does not satisfy a contract
// assertion's requires-clause does not have the assertion, and its predicate
// is never checked, on every route -- function templates (bare and
// parenthesized clauses, a requires-expression naming a parameter), a
// substitution failure in the clause, a settled conjunction or disjunction,
// contract_assert (a discarded one as an if's substatement), members and
// member templates of a class template, a postcondition with a label,
// attributes and a result name, a generic lambda, and a member of a local
// class in a function template.  (From the EDG fork's
// p4283/requires_constexpr.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-requires-constexpr-routes.C,
// which also has a non-generic lambda in a function template.

template <class T> struct is_int { static constexpr bool value = false; };
template <> struct is_int<int> { static constexpr bool value = true; };
template <class T> concept Int = is_int<T>::value;
template <class T> concept Big = sizeof(T) > 100;
struct Label { using assertion_control_object = Label; };
constexpr Label lbl{};

// Function templates: the bare and parenthesized forms, a conjunction of a
// concept and a requires-expression naming a parameter.
template <class T>
constexpr T f1(T x) pre requires Int<T> (x > 0) { return x; }  // expected-error {{contract failed during execution of constexpr function}}
static_assert(f1(1.5) == 1.5);   // OK, discarded
constexpr int e1 = f1(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

template <class T>
constexpr T f2(T x)
  pre requires (Int<T> && requires { x + 1; }) (x > 0) { return x; }  // expected-error {{contract failed during execution of constexpr function}}
static_assert(f2(-1.5) == -1.5); // OK, discarded
constexpr int e2 = f2(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A substitution failure makes the constraint not satisfied, and the
// predicate, invalid for that instance, is not scanned; a settled
// conjunction or disjunction does not substitute its second operand.
template <class T>
constexpr int s1(T t) pre requires (T::value > 0) (t < T::value) { return 1; }
template <class T>
constexpr int s2(T t) pre requires (Big<T> && T::value > 0) (t > 0) { return 1; }
template <class T>
constexpr int s3(T t) pre requires (!Big<T> || T::value > 0) (t < 0) { return 1; }  // expected-error {{contract failed during execution of constexpr function}}
static_assert(s1(1) + s2(1) == 2);   // OK, both discarded
constexpr int e10 = s3(1);           // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// contract_assert; a discarded one is an empty statement.
template <class T>
constexpr int m1(T t) {
  contract_assert requires (T::value > 0) (t.missing());
  if (t) contract_assert requires Int<T> (t > 0);  // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(m1(1.5) == 1);     // OK, both discarded
constexpr int e3 = m1(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// Members of a class template (cached), naming parameters, and a member
// template; a postcondition with a result name and a label.
template <class T>
struct S {
  constexpr int pre1(T x) const
    pre requires (sizeof(x) <= 4 && requires { x + 1; }) (x > 0) { return 1; }  // expected-error {{contract failed during execution of constexpr function}}
  template <class U>
  constexpr int pre2(U y) const pre requires Int<U> (y > 0) { return 1; }  // expected-error {{contract failed during execution of constexpr function}}
  constexpr T post1(T x) const
    post <lbl> requires Int<T> (r [[maybe_unused]]: r > 0) { return x; }  // expected-error {{contract failed during execution of constexpr function}}
};
static_assert(S<double>().pre1(-1.0) == 1);  // OK, discarded
constexpr int e4 = S<int>().pre1(-1);        // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
static_assert(S<int>().pre2(-1.0) == 1);     // OK, discarded
constexpr int e5 = S<int>().pre2(-1);        // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
static_assert(S<double>().post1(-1.0) == -1.0);  // OK, discarded
constexpr int e6 = S<int>().post1(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A generic lambda.  (A lambda in a function template whose only contract
// specifier is discarded is a separate open bug.)
constexpr auto gl = [](auto y) pre requires Int<decltype(y)> (y > 0) {  // expected-error {{contract failed during execution of constexpr function}}
  return 1;
};
static_assert(gl(-1.5) == 1);    // OK, discarded
constexpr int e8 = gl(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}

// A member of a local class in a function template.
template <class T>
constexpr int c1(T t) {
  struct L { constexpr int m(T x) pre requires Int<T> (x > 0) { return 1; } };  // expected-error {{contract failed during execution of constexpr function}}
  return L().m(t); // expected-note {{in call to}}
}
static_assert(c1(-1.5) == 1);    // OK, discarded
constexpr int e9 = c1(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
