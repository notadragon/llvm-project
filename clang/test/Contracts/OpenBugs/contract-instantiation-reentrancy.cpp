// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// XFAIL: *

// OPEN BUG (CLANG-13 in this fork's open-issues/): instantiating one
// function's contracts from inside another contract's predicate trips
//
//   SemaContract.cpp: Sema::PushContractScope(...):
//   Assertion `LastScope && !LastScope->isInContract()' failed.
//
// InstantiateFunctionContractsOnUse is called from MarkFunctionReferenced for
// every odr-use, and a contract predicate can odr-use a function that itself
// has contracts -- so contract instantiation re-enters itself while the outer
// contract scope is still open, and PushContractScope's invariant does not
// hold.  Introduced by 0806c848467e (2026-09-04); found 2026-09-05 by a BDE
// rebuild, where it crashed 42 translation units in bslstl.
//
// THIS ROW IS CLANG-ONLY.  Measured 2026-09-05: GCC compiles the same source
// clean with -fcontracts and with -fcontracts-p3850, so the gnu_gcc mirror
// expects success rather than xfailing.  Same content, opposite expectation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-contract-instantiation-reentrancy.C

// expected-no-diagnostics

template <class T>
struct S {
  bool ok() const pre(n >= 0) { return n >= 0; }
  int n = 0;
};

// THE OPEN BUG: the predicate calls a contracted member, so transforming it
// re-enters contract instantiation.
template <class T>
int f(S<T> s) pre(s.ok()) { return s.n; }

int use_f() { return f(S<int>{}); }

// Control: a predicate that reaches no contracted function is fine, which is
// what places the defect in the re-entrancy and not in contracts on templates.
template <class T>
int g(S<T> s) pre(s.n >= 0) { return s.n; }

int use_g() { return g(S<int>{}); }
