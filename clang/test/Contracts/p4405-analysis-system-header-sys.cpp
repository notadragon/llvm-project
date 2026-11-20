// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -O0 -Wsystem-headers -Wno-everything -Wcontract-unproven %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// p4405-analysis-system-header.cpp with -Wsystem-headers: the warnings for
// the assertions that are themselves in the system header, hidden there,
// are shown -- the header's own closure analyzed at sys_invoke's call (line
// 20 of the header) and sys_callee's precondition analyzed at sys_caller's
// call (line 32) -- so that test's silence is the system-header rule's, not
// a proof.  -Wno-everything -Wcontract-unproven keeps libc++'s own warnings
// quiet.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-system-header-sys.C

#include "p4405-analysis-system-header.h"

void
use(int n)
{
  auto ld = [](int x) pre<always_diagnose>(x > 0) {};  // expected-note {{the predicate of the contract assertion}}
  sys_invoke(ld, n);  // expected-note {{in instantiation of function template specialization 'sys_invoke<}}
  sys_caller(n);  // expected-note {{in instantiation of function template specialization 'sys_caller<int>'}}
  sys_lambda_caller(n);
  sys_callee(n);  // expected-warning {{contract assertion cannot be proven}}
}

// expected-warning@p4405-analysis-system-header.h:20 2 {{contract assertion cannot be proven}}
// expected-warning@p4405-analysis-system-header.h:32 {{contract assertion cannot be proven}}
// expected-note@p4405-analysis-system-header.h:24 2 {{the predicate of the contract assertion}}
// expected-note@p4405-analysis-system-header.h:38 {{the predicate of the contract assertion}}
// expected-note@p4405-analysis-system-header.h:39 {{in instantiation of function template specialization 'sys_invoke<}}
