// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation") and notadragon_wg21 DECISIONS.md E186
// (a): a diagnose warning whose analysis evaluation is in a system header
// is shown when the contract assertion is the program's -- a closure's
// precondition analyzed where a system header's forwarding template calls
// it -- located at that call, with the instantiation trail into this file.
// A warning for an assertion that is itself in a system header stays
// hidden without -Wsystem-headers: sys_caller's call of sys_callee, and the
// header's own closure passed to sys_invoke.  A call of sys_callee made
// here is located here and warned about as any other.  Compiled at proof level 0;
// the warnings are raised where code generation meets the evaluation,
// never after inlining.  p4405-analysis-indirect-lambda.cpp has the libc++
// form, and p4405-analysis-system-header-sys.cpp shows the hidden warnings
// with -Wsystem-headers.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-system-header.C

#include "p4405-analysis-system-header.h"

void
use(int n)
{
  auto ld = [](int x) pre<always_diagnose>(x > 0) {};  // expected-note {{the predicate of the contract assertion}}
  sys_invoke(ld, n);  // expected-note {{in instantiation of function template specialization 'sys_invoke<}}
  sys_caller(n);
  sys_lambda_caller(n);
  sys_callee(n);  // expected-warning {{contract assertion cannot be proven}}
}

// The closure's precondition, analyzed at sys_invoke's call of it (line 20
// of the header), and the note for sys_callee's predicate.  The header's
// own closure, analyzed there too, and sys_callee's precondition analyzed
// at sys_caller's call (line 32) are not reported.
// expected-warning@p4405-analysis-system-header.h:20 {{contract assertion cannot be proven}}
// expected-note@p4405-analysis-system-header.h:24 {{the predicate of the contract assertion}}
