// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): a closure passed to std::function or
// std::invoke is not converted to a pointer; the library calls its function
// call operator directly, and that call has the analysis evaluation of the
// operator's preconditions, with arguments the library does not know.  So
// an always_prove precondition cannot be proven there -- an error, located
// in the library -- and an always_diagnose one draws a warning there.  A
// closure passed to a forwarding template of the program's own is analyzed
// at the template's call in the same way.
//
// libc++'s headers are system headers, but ld's precondition is this
// file's, so the warning for ld's call in them is shown without
// -Wsystem-headers (notadragon_wg21 DECISIONS.md E186 (a)), with the
// instantiation trail into this file -- once: std::function and
// std::invoke both call it through the same specialization of
// std::__invoke.  The error for lp's call is shown as any error is.
// p4405-analysis-system-header.cpp covers a system header of the
// testsuite's own, and an assertion that is itself in a system header.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-indirect-lambda.C

#include <contracts>
#include <functional>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template <typename F, typename... A>
void my_invoke(F &&f, A &&...a)
{
  f(static_cast<A &&>(a)...);  // expected-warning {{contract assertion cannot be proven}}
}

void
use()
{
  auto lp = [](int x) pre<always_prove>(x > 0) {};  // expected-note {{the predicate of the contract assertion}}
  std::function<void(int)> g1 = lp;  // expected-note {{in instantiation of function template specialization 'std::function<void (int)>::function<}}
  g1(1);
  auto ld = [](int x) pre<always_diagnose>(x > 0) {};  // expected-note 2 {{the predicate of the contract assertion}}
  std::function<void(int)> g2 = ld;
  std::invoke(ld, 2);  // expected-note {{in instantiation of function template specialization 'std::invoke<}}
  my_invoke(ld, 3);  // expected-note {{in instantiation of function template specialization 'my_invoke<}}
  g2(1);
}

// The error inside libc++ (std::function's call of lp), the warning there
// (the call of ld), and the frames of their instantiation there.  These
// come last, after the directives for this file's lines.
// expected-error@*:* {{contract assertion cannot be proven}}
// expected-warning@*:* {{contract assertion cannot be proven}}
// expected-note@*:* 1+ {{in instantiation of}}
