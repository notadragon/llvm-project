// DEFINE: %{cc} = %clangxx -std=c++26 -fcontract-evaluation-semantic=ignore %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451
// REQUIRES: contracts-libcxx

// P4451: constant evaluation of a terminating contract assertion uses a
// terminating semantic too: with ignore configured, a false predicate makes
// the evaluation fail.  Under -fcontracts-p4451 alone, -fcontracts-p3400
// alone (the P3400 label) and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-constexpr.C (and its
// -p3400 and -both variants)

#include <contracts>  // `terminating`

constexpr int pre_f(int x) pre<terminating>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int post_f(int x) post<terminating>(r : r > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int assert_f(int x) {
  contract_assert<terminating>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return x;
}
struct S {
  static constexpr int mem(int x) pre<terminating>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
};
template <typename T>
constexpr T tmpl(T x) pre<terminating>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr auto lam = [](int x) pre<terminating>(x > 0) { return x; }; // expected-error {{contract failed during execution of constexpr function}}

constexpr int a = pre_f(1);
constexpr int b = pre_f(-1);     // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int c = post_f(-1);    // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int d = assert_f(-1);  // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int e = S::mem(-1);    // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int f = tmpl(-1);      // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
constexpr int g = lam(-1);       // expected-error {{must be initialized by a constant expression}} expected-note {{in call to}}
