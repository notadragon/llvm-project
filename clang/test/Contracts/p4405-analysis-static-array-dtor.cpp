// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O0 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// DECISIONS.md E148 (b): an array of static or thread storage duration is
// destroyed at exit by a function the implementation generates
// (__cxx_global_array_dtor, or a destroy helper for a temporary), which
// calls the destructor for each element directly, and those calls are
// analyzed like any other.  So besides the address-of diagnostic its
// definition may get (D4405R0 "Taking the Address of a Function",
// p4405-analysis-address-static-dtor.cpp, and for a temporary
// p4405-analysis-address-static-temp.cpp), code generation reports the
// destructor precondition's analysis evaluation: "cannot be proven", an
// error under prove and a -Wcontract-unproven warning under diagnose.  A
// non-array object has no such call.
//
// The prove case here is a precondition analyzed with prove through
// -fcontract-analysis-semantic=prove, whose runtime semantic stays
// enforce, so the definition gets no address-of diagnostic -- only the
// configuration selects prove -- and only the generated calls are
// reported; always_diagnose (`DD') gets the address-of warning as well.
// Under always_prove the address-of diagnostic is an error, and code
// generation does not run.  (CLANG-864.)

#include <contracts>
#include <initializer_list>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DI {
  ~DI() pre(g > 0); // expected-note 7 {{the predicate of the contract assertion}}
};
struct DD {
  ~DD() pre<always_diagnose>(g > 0); // expected-note 2 {{precondition here}} expected-note 2 {{the predicate of the contract assertion}}
};

DI ia[2]; // expected-error {{contract assertion cannot be proven}}
DD da[2]; // expected-warning {{contract assertion cannot be proven for a call through the address of '~DD'}} expected-note {{'da', of static storage duration, is destroyed by a call through the address of its destructor}} expected-warning {{contract assertion cannot be proven}}
thread_local DI ta[2]; // expected-error {{contract assertion cannot be proven}}
DI ma[2][3]; // expected-error {{contract assertion cannot be proven}}

// Not an array, and only the configuration selects prove: nothing.
DI i1;

struct S {
  static DI sa[2];
};
DI S::sa[2]; // expected-error {{contract assertion cannot be proven}}

void f() {
  static DI sl[2]; // expected-error {{contract assertion cannot be proven}}
  static DD sd[3]; // expected-warning {{contract assertion cannot be proven for a call through the address of '~DD'}} expected-note {{'sd', of static storage duration, is destroyed by a call through the address of its destructor}} expected-warning {{contract assertion cannot be proven}}
}

// Temporaries of static storage duration (E147 (a)): an array, and the
// backing array of a std::initializer_list.
const DI (&ra)[2] = {DI(), DI()}; // expected-error {{contract assertion cannot be proven}}
std::initializer_list<DI> il = {DI(), DI()}; // expected-error {{contract assertion cannot be proven}}
