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
// libc++'s headers are system headers, so the warnings for ld's calls in
// them (std::function's and std::invoke's) are not shown, as no warning in
// a system header is; the error for lp's is.  (gnu_gcc's testsuite headers
// are not system headers, so it shows both.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-indirect-lambda.C

#include <contracts>
#include <functional>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

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
  auto ld = [](int x) pre<always_diagnose>(x > 0) {};  // expected-note {{the predicate of the contract assertion}}
  std::function<void(int)> g2 = ld;
  std::invoke(ld, 2);
  my_invoke(ld, 3);  // expected-note {{in instantiation of function template specialization 'my_invoke<}}
  g2(1);
}

// The error inside libc++ (std::function's call of lp), and the frames of
// its instantiation there.  These come last, after the directives for this
// file's lines.
// expected-error@*:* {{contract assertion cannot be proven}}
// expected-note@*:* 1+ {{in instantiation of}}
