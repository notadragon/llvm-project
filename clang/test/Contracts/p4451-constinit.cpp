// DEFINE: %{cc} = %clangxx -std=c++26 -fcontract-evaluation-semantic=ignore %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451
// REQUIRES: contracts-libcxx

// P4451: whether a variable is constant-initialized is decided with every
// contract assertion using the ignore semantic ([expr.const.init]), so the
// initializer below is a constant initializer; its evaluation then uses a
// terminating semantic, and the false predicate makes it fail.  Under
// -fcontracts-p4451 alone, -fcontracts-p3400 alone (the P3400 label) and
// both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-constinit.C (and its
// -p3400 and -both variants)

#include <contracts>  // `terminating`

constexpr int l(int x) pre<terminating>(x != 1) { return x; } // expected-error {{contract failed during execution of constexpr function}}
const int i1 = l(1); // expected-error {{initialization of constant-initialized variable failed}} expected-note {{initializer is constant when evaluated with 'ignore'}} expected-note {{in call to}}
