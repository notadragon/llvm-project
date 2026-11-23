// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -O2 -g \
// RUN:   %libcxx_flags -c -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -O0 \
// RUN:   %libcxx_flags -c -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fcontracts-p4405 -O2 -g \
// RUN:   -fcontracts-prove-optimize=0 %libcxx_flags -c -o /dev/null \
// RUN:   -Xclang -verify=expected,fold %s
// REQUIRES: contracts-libcxx

// D4405: proving a pointer not null where the pointer is a string literal
// passed by the caller.  A contract_assert is evaluated in the function that
// contains it, so its analysis evaluation proves the predicate there alone:
// for a parameter, whatever the callers pass -- a string literal through a
// local pointer, at -O2 with debug information -- it cannot be proven, even
// for an internal function whose only caller passes one.  A precondition's
// analysis evaluation is made at each call, in the caller, where the
// literal's address is known not to be null, so it is proven: by folding for
// the literal itself, and through the local pointer by the proof shadow, so
// not with -fcontracts-prove-optimize=0.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove-string-literal-arg.C
// and p4405-prove-string-literal-arg-fold.C

#include <contracts>  // `always_prove`, `always_diagnose`

using namespace std::contracts::controls;

void f_str2(const char *str) {
  contract_assert<always_prove>(str != nullptr); // expected-error {{contract assertion cannot be proven}}
  __builtin_printf("%s\n", str);
}

void f_str1(const char *str) { f_str2(str); }

static void s_str(const char *str) {
  contract_assert<always_prove>(str != nullptr); // expected-error {{contract assertion cannot be proven}}
  __builtin_printf("%s\n", str);
}

void d_str(const char *str) {
  contract_assert<always_diagnose>(str != nullptr); // expected-warning {{contract assertion cannot be proven}}
  __builtin_printf("%s\n", str);
}

void f_pre(const char *str) pre<always_prove>(str != nullptr); // fold-note {{the predicate of the contract assertion}}

void g(int) {
  const char *ptr = "message";
  f_str2(ptr);
  s_str(ptr);
  d_str(ptr);
  f_pre(ptr);       // fold-error {{contract assertion cannot be proven}}
  f_pre("literal"); // OK, proven
}
