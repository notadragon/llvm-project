// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s

// P4283: Redeclaration matching for requires clauses.

template<typename T>
concept Integral = __is_integral(T);

template<typename T>
concept Signed = __is_signed(T);

// Same requires clause — OK.
template<typename T>
T f(T x) pre requires(Integral<T>) (x > 0);
template<typename T>
T f(T x) pre requires(Integral<T>) (x > 0) { return x; }

// Different requires clause — error.
template<typename T>
T g(T x) pre requires(Integral<T>) (x > 0); // expected-note {{contract previously specified with a non-equivalent requires clause}}
template<typename T>
T g(T x) pre requires(Signed<T>) (x > 0) { return x; } // expected-error {{differs in contract specifier sequence}} \
                                                         // expected-note {{in contract specified here}}
