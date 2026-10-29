// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("Taking the Address of a Function"): a default argument that
// takes the address of a function takes it at each call that uses the
// default argument, and its semantic constraints are checked where it is
// written ([dcl.fct.default]), so that is where a precondition that cannot
// be proven for a call through the address is diagnosed -- an error for
// always_prove, a -Wcontract-unproven warning for always_diagnose -- once,
// whether or not a call uses it, and not again at the calls: whether it
// takes the address by the unary & or by the function-to-pointer
// conversion or the reference binding that converts it to the parameter's
// type.  A default argument of a template is diagnosed where it is
// instantiated (for a call that uses it), as everything in a template is;
// one never instantiated is not.  A default argument in an unevaluated
// operand takes no address.
// (CLANG-855: a default argument was diagnosed nowhere.)  gnu_gcc locates
// a template's implicitly converted default argument (t2) at the
// instantiating call, Clang at the default argument, with a note at the
// call.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-address-default-arg.C

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

void fp(int x) pre<always_prove>(x > 0);   // expected-note 10 {{precondition here}}
void fd(int x) pre<always_diagnose>(x > 0); // expected-note 3 {{precondition here}}

void a(void (*p)(int) = &fp);  // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
void b(void (*p)(int) = fp);   // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
void c(void (*p)(int) = fd);   // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
void d(void (&r)(int) = fp);   // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
void unused(void (*p)(int) = &fd); // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
void unused2(void (*p)(int) = fd); // expected-warning {{contract assertion cannot be proven for a call through the address of 'fd'}}
void e(unsigned long n = sizeof(&fp));

struct S {
  void m(void (*p)(int) = &fp); // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
  void m2(void (*p)(int) = fp); // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
};

template <class T> void t(void (*p)(int) = &fp); // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
template <class T> void t2(void (*p)(int) = fp); // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
template <class T> void never(void (*p)(int) = &fp);
template <class T> void never2(void (*p)(int) = fp);

template <class T> struct U {
  void m(void (*p)(int) = &fp); // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
};

void use(S &s, U<int> &u) {
  a();
  a();
  b();
  b();
  c();
  d();
  e();
  s.m();
  s.m2();
  t<int>(); // expected-note {{in instantiation of default function argument expression for 't<int>' required here}}
  t<int>();
  t2<int>(); // expected-note {{in instantiation of default function argument expression for 't2<int>' required here}}
  t2<int>();
  u.m(); // expected-note {{in instantiation of default function argument expression for 'm<int>' required here}}
  auto l = [](void (*p)(int) = &fp) {}; // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
  l();
  auto l2 = [](void (*p)(int) = fp) {}; // expected-error {{contract assertion cannot be proven for a call through the address of 'fp'}}
  l2();
}
