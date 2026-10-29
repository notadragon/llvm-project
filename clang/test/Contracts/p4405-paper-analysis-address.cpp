// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 -fcontract-analysis-semantic=prove %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Taking the Address of a Function" (Implementation Experience;
// D4405R0-implexample-address-variations): diagnosed only where it is known
// that calls through the pointer need a proof, when the precondition's
// labels allow no runtime semantic.  f1's always_prove precondition folds
// to true, so it is proven tautological and nothing is reported; f2's
// always_prove precondition is not tautological, an error; f3's
// always_diagnose one, a warning; f4's precondition is analyzed with prove
// only because the configuration says so, so nothing is reported and calls
// through p4 go unanalyzed.  The definition of s takes the address of
// S::~S, which the runtime calls at exit: an error.  The paper's example,
// with its comments, built as the paper says (-O2,
// -fcontract-analysis-semantic=prove).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-paper-analysis-address.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// The paper's always_prove label, and always_diagnose, "defined the same
// way".
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void f1(int x) pre<always_prove>(sizeof(x) == sizeof(int));
void f2(int x) pre<always_prove>(x > 0);  // expected-note {{precondition here}}
void f3(int x) pre<always_diagnose>(x > 0);  // expected-note {{precondition here}}
void f4(int x) pre(x > 0);  // Analyzed with prove by the configuration

int n;

struct S {
  ~S() pre<always_prove>(n > 0);  // expected-note {{precondition here}}
};

S s;  // Error, cannot be proven for the call at exit expected-error {{contract assertion cannot be proven for a call through the address of '~S'}} expected-note {{'s', of static storage duration, is destroyed by a call through the address of its destructor}}

void g()
{
  void (*p1)(int) = &f1;  // OK, proven tautological
  void (*p2)(int) = &f2;  // Error, cannot be proven through `p2` expected-error {{contract assertion cannot be proven for a call through the address of 'f2'}}
  void (*p3)(int) = &f3;  // OK, warns: cannot be proven through `p3` expected-warning {{contract assertion cannot be proven for a call through the address of 'f3'}}
  void (*p4)(int) = &f4;  // OK, calls through `p4` are not analyzed
}
