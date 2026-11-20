// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only %s
//
// RUN: %{cc} -fcontracts-p4451 -fcontract-evaluation-semantic=ignore -Xclang -verify=ignore
// RUN: %{cc} -fcontracts-p4451 -fcontract-evaluation-semantic=enforce -Xclang -verify=enforce
// REQUIRES: contracts-libcxx

// ignore-no-diagnostics

// P4451R3: constant evaluation of a contract assertion whose specifier names
// std::contracts::controls::defaulted uses the configured semantic, as an
// unlabeled one does (p4451-constexpr.cpp is the contrast for enforced):
// with ignore configured, a false predicate leaves the evaluation constant;
// with enforce, it fails.  Under -fcontracts-p4451 alone, -fcontracts-p3400
// alone (the empty P3400 label) and both.

#include <contracts>  // `std::contracts::controls::defaulted`

using namespace std::contracts::controls;

constexpr int pre_f(int x) pre<defaulted>(x > 0) { return x; } // enforce-error {{contract failed during execution of constexpr function}}
constexpr int post_f(int x) post<defaulted>(r : r > 0) { return x; } // enforce-error {{contract failed during execution of constexpr function}}
constexpr int assert_f(int x) {
  contract_assert<defaulted>(x > 0); // enforce-error {{contract failed during execution of constexpr function}}
  return x;
}
template <typename T>
constexpr T tmpl(T x) pre<defaulted>(x > 0) { return x; } // enforce-error {{contract failed during execution of constexpr function}}

constexpr int a = pre_f(1);
constexpr int b = pre_f(-1);     // enforce-error {{must be initialized by a constant expression}} enforce-note {{in call to}}
constexpr int c = post_f(-1);    // enforce-error {{must be initialized by a constant expression}} enforce-note {{in call to}}
constexpr int d = assert_f(-1);  // enforce-error {{must be initialized by a constant expression}} enforce-note {{in call to}}
constexpr int e = tmpl(-1);      // enforce-error {{must be initialized by a constant expression}} enforce-note {{in call to}}
