// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function"): a call through a pointer or
// a reference to a function cannot identify the function, so it has no
// analysis evaluation and a precondition can be proven for it only if it is
// tautological.  Taking the address of a function -- the unary &, the
// function-to-pointer conversion, forming a pointer to member function,
// binding a reference -- is diagnosed when a precondition of it allows no
// runtime semantic and folding does not prove it: "contract assertion
// cannot be proven for a call through the address of", an error under
// prove (always_prove) and a -Wcontract-unproven warning under diagnose
// (always_diagnose).  A precondition that is not analyzed draws nothing,
// nor does an unevaluated operand, nor a virtual call (P3097), which
// analyzes the precondition of the function it names.
// Diagnosed in Sema, so -fsyntax-only shows it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address.C

#include <contracts>

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

void fp(int x) pre<always_prove>(x > 0);    // expected-note 5 {{precondition here}}
void fd(int x) pre<always_diagnose>(x > 0); // expected-note {{precondition here}}
void fn(int x) pre(x > 0);
void fo(int x);
void fo(double x) pre<always_prove>(x > 0); // expected-note {{precondition here}}

struct S {
  void mp(int x) pre<always_prove>(x > 0);                // expected-note {{precondition here}}
  static void sd(int x) pre<always_diagnose>(x > 0);      // expected-note 2 {{precondition here}}
  virtual void vd(int x) pre<always_diagnose>(x > 0);     // expected-note {{precondition here}}
};

template <typename T>
void tp(T x) pre<always_prove>(x > 0); // expected-note {{precondition here}}

void (*gp)(int) = fp; // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}

void use(S &s) {
  void (*p1)(int) = fp;   // expected-error {{cannot be proven for a call through the address of 'fp'}}
  void (*p2)(int) = &fp;  // expected-error {{cannot be proven for a call through the address of 'fp'}}
  void (&r1)(int) = fp;   // expected-error {{cannot be proven for a call through the address of 'fp'}}
  void (*p3)(int) = fd;   // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
  void (*p4)(int) = fn;
  void (*p5)(int) = fo;
  void (*p6)(double) = fo; // expected-error {{cannot be proven for a call through the address of 'fo'}}
  auto pm = &S::mp;        // expected-error {{cannot be proven for a call through the address of 'mp'}}
  void (*ps)(int) = &S::sd; // expected-warning {{cannot be proven for a call through the address}}
  void (*ps2)(int) = s.sd;  // expected-warning {{cannot be proven for a call through the address}}
  auto pv = &S::vd;         // expected-warning {{cannot be proven for a call through the address}}
  void (*pt)(int) = tp<int>; // expected-error {{cannot be proven for a call through the address of 'tp<int>'}}
  decltype(&fp) q = nullptr;
  (void)sizeof(&fp);
  (void)noexcept(&fp);
  (void)p1; (void)p2; (void)r1; (void)p3; (void)p4; (void)p5;
  (void)p6; (void)pm; (void)ps; (void)ps2; (void)pv; (void)pt; (void)q;
}

void call(S &s) {
  s.vd(1);
  fp(1);
  S::sd(1);
  s.sd(1);
  (fp)(1);
}

// In a template, at instantiation only.
template <typename T> void in_template() {
  void (*p)(int) = fp; // expected-error {{cannot be proven for a call through the address of 'fp'}}
  (void)p;
}
template void in_template<int>(); // expected-note {{in instantiation of function template specialization 'in_template<int>' requested here}}
