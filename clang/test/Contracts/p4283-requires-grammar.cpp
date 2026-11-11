// P4283: a requires-clause on a contract uses the *standard* requires-clause
// grammar -- `requires constraint-logical-or-expression` -- with no mandatory
// parentheses around the constraint.  The paper's own example is
//   pre requires std::integral<T> (x > 0)
// The constraint's atoms are primary-expressions, so parsing stops before the
// contract predicate's '('.  Negation/comparison atoms need parentheses exactly
// as in an ordinary requires-clause.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fcontracts-p4283 -fsyntax-only -verify %s
// expected-no-diagnostics

template <class T> concept Small = sizeof(T) <= 4;
template <class T> concept Big = sizeof(T) >= 100;
template <class T> concept Any = true;

// Bare concept-id constraint, no parentheses (the paper's primary form).
template <class T> T a(T x) pre requires Small<T> (x > 0) { return x; }

// Grouped constraint (parentheses as a primary-expression).
template <class T> T b(T x) pre requires (Small<T>) (x > 0) { return x; }

// Negation and comparison atoms require their own parentheses (standard rule).
template <class T> T c(T x) pre requires (!Big<T>) (x > 0) { return x; }
template <class T> T d(T x) pre requires (sizeof(T) >= 1) (x > 0) { return x; }

// Conjunction / disjunction of concept-ids.
template <class T> T e(T x) pre requires Small<T> && Any<T> (x > 0) { return x; }
template <class T> T f(T x) pre requires Small<T> || Big<T> (x > 0) { return x; }

// Postcondition (with a result name) and assertion-statement forms.
template <class T> T g(T x) post requires Small<T> (r : r > 0) { return x; }
template <class T> T h(T x) {
  contract_assert requires Small<T> (x > 0);
  return x;
}

// Instantiate so the constraints are actually evaluated.
template int a<int>(int);
template int b<int>(int);
template int c<int>(int);
template int d<int>(int);
template int e<int>(int);
template int f<int>(int);
template int g<int>(int);
template int h<int>(int);
