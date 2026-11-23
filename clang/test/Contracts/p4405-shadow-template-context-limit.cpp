// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=3 -O0 -emit-llvm-only -verify=expected,limited %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=3 -O2 -emit-llvm-only -verify=expected,limited %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=3 -O2 -fno-contracts-prove-shadow -emit-llvm-only -verify=expected,limited %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=0 -O2 -emit-llvm-only -verify=expected,all %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -triple x86_64-unknown-linux-gnu -fcontract-evaluation-semantic=diagnose -ftemplate-backtrace-limit=3 -O2 -clear-ast-before-backend -emit-llvm-only -verify=expected,limited %s
// REQUIRES: x86-registered-target

// P4405 stage 3: the instantiation notes of a diagnostic the proof shadows
// issue after the proof module honour -ftemplate-backtrace-limit as Sema's
// do: with a limit of 3 of five levels, the two innermost, a note saying
// how many were skipped (at the first one skipped, c<1>'s point of
// instantiation), and the outermost; with 0, all five.  diagnose:
// the diagnostic is a warning.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-template-context-limit.C
// (each prints its own compiler's backtrace and keeps its own levels)

template <int N> int a (int x) { contract_assert (x > N); return x; }  // expected-warning {{contract assertion cannot be proven}}
template <int N> int b (int x) { return a<N> (x); }  // expected-note {{in instantiation of function template specialization 'a<1>' requested here}}
template <int N> int c (int x) { return b<N> (x); }  // expected-note {{in instantiation of function template specialization 'b<1>' requested here}}
template <int N> int d (int x) { return c<N> (x); }  // all-note {{in instantiation of function template specialization 'c<1>' requested here}} limited-note {{(skipping 2 contexts in backtrace; use -ftemplate-backtrace-limit=0 to see all)}}
template <int N> int e (int x) { return d<N> (x); }  // all-note {{in instantiation of function template specialization 'd<1>' requested here}}
int z (int x) { return e<1> (x); }  // expected-note {{in instantiation of function template specialization 'e<1>' requested here}}
