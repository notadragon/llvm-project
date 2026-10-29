// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-p3098 -fcontract-configuration-file=%S/p4405-template-caller-post.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405 stage 2: a caller-side evaluation of a postcondition of a
// specialization of a template with a definition is folded with the
// predicate substituted, as p4405-template-caller.cpp does for a
// precondition: a predicate that the template arguments decide is proven
// or always false.  One naming the result, one holding a lambda, and a
// dependent one of a postcondition with captures stay unproven.  The
// callee-side evaluations are ignore, so the definitions are not
// diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-caller-side-template-post.C
// (gnu_gcc, P5b-GCC).

template <class U>
U lit(U u) post(true) { return u; }

template <class U>
U small(U u) post(sizeof(U) <= 8) { return u; } // expected-note {{the predicate of the contract assertion}}

template <class U>
U res(U u) post(r: r > 0) { return u; } // expected-note {{the predicate of the contract assertion}}

template <class U>
U lam(U u) post(r: [](U v) { return v > 0; }(r)) { return u; } // expected-note {{the predicate of the contract assertion}}

template <class U>
U cap(U u) post[s = sizeof(U)](s <= 8) { return u; } // expected-note {{the predicate of the contract assertion}}

struct Big { char c[100]; };

void use(int n) {
  lit(n);                       // proven
  small(n);                     // proven
  small(Big{});                 // expected-error {{contract predicate is always false}}
  res(1);                       // expected-error {{contract assertion cannot be proven}}
  lam(1);                       // expected-error {{contract assertion cannot be proven}}
  cap(n);                       // expected-error {{contract assertion cannot be proven}}
}
