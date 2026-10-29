// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -O0 -emit-llvm-only -verify=expected,unshadowed %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -O2 -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -O2 -clear-ast-before-backend -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -O2 -fno-contracts-prove-shadow -emit-llvm-only -verify=expected,unshadowed %s
// REQUIRES: x86-registered-target

// P4405 stage 3: a caller-side evaluation of a precondition in a template
// instantiation is followed by the instantiation notes of the caller, then
// the note at the predicate (Sema's order: the instantiation notes come
// right after the diagnostic) -- diagnosed by the caller's proof shadow
// after the proof module or at once without one.  The shadow proves it
// where the instantiation tests the argument; without one both calls are
// diagnosed, consecutively in one instantiation, so the instantiation notes
// are printed once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-context-caller.C
// (each prints its own compiler's backtrace; GCC's runs at -O2 only)

void take (const char *p)
  pre (p)  // expected-note {{the predicate of the contract assertion}} unshadowed-note {{the predicate of the contract assertion}}
{
}

template <class T>
void use (T p)
{
  if (p)
    take (p);  // unshadowed-error {{contract assertion cannot be proven}}
  take (p);  // expected-error {{contract assertion cannot be proven}}
}

void call (const char *p)
{
  use (p);  // expected-note {{in instantiation of function template specialization 'use<const char *>' requested here}}
}
