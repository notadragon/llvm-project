// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=prove %libcxx_flags -DCODEGEN -S -emit-llvm -o /dev/null -Xclang -verify=codegen %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=prove %libcxx_flags -fsyntax-only -Xclang -verify %s

// D4405 ("Adjusting to the Allowed Semantics", "Analyzing Assertions With
// Restricted Semantics"): the standard labels when the build configures
// prove for the runtime evaluations, which step 2 moves to the analysis
// evaluation.  symbolic now allows prove, so its assertions are proven and
// still never checked; always_ignore allows neither, so they are never
// analyzed.  terminating allows prove too, but its runtime evaluations are
// still adjusted to enforce (step 5, which never reaches prove), as the
// constant evaluation shows -- configuration never lets a check be skipped
// when the label allows a runtime semantic.  (A Sema error stops code
// generation, which diagnoses prove, so the two halves are compiled
// separately.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-labels-prove.C

#include <contracts>

namespace L = std::contracts::labels;

#ifdef CODEGEN
int a(int x) pre<L::symbolic>(x > 0) { return x; }      // codegen-note {{the predicate of the contract assertion}}
int b(int x) pre<L::always_ignore>(x > 0) { return x; }
int c() pre<L::symbolic>(true) { return 0; }
void ae2_calls(int y) {
  a(y); // codegen-error {{contract assertion cannot be proven}}
  b(y);
  c();
}
#else
constexpr int t(int x) pre<L::terminating>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int good = t(1);
constexpr int bad = t(-1); // expected-error {{constexpr variable 'bad' must be initialized by a constant expression}}
  // expected-note@-1 {{in call to 't(-1)'}}
#endif

// REQUIRES: contracts-libcxx
