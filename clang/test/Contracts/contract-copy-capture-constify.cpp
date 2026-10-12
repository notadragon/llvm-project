// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// In a contract_assert inside a mutable lambda, a name referring to a by-copy
// capture is const: the capture is a member of the closure object and a
// contract predicate constifies it, and the message says the capture is by
// copy.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-copy-capture-constify.C

void a(int x) { [=]() mutable { contract_assert(++x > 0); (void)x; }(); } // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
void b(int x) { [x]() mutable { contract_assert(++x > 0); }(); }          // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
void c(int x) { [y = x]() mutable { contract_assert(++y > 0); }(); }      // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
void d() { int x = 0; [x]() mutable { contract_assert(++x > 0); }(); }    // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
template <class T>
void e(T x) { [x]() mutable { contract_assert(++x > 0); }(); }            // expected-error {{cannot assign to a variable captured by copy which was captured as const because it is inside a contract}}
// expected-note@-1 {{while substituting into a lambda expression here}}
template void e(int); // expected-note {{in instantiation of}}
