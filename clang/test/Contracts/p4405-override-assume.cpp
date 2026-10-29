// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove %libcxx_flags -O2 -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "Virtual Functions": no override proof is ever an assumption in
// the real code (-fcontracts-prove-assume, implied by -fcontracts-p4405):
// the overrider also serves calls that never established the overridden
// function's preconditions.  D1::f's precondition, whose runtime
// evaluation is ignore, is proven soundly from B1::f's, whose label
// settles it (DECISIONS.md E151) -- its ignored runtime evaluation after
// the obligation assumes nothing there -- yet the real D1::f keeps its
// test of x.  An ordinary sound proof next to an override shadow is still
// assumed: D2::f's contract_assert, which D2::f's override shadow takes as
// a premise.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-assume.C
// (Clang's proof shadow takes a precondition's ignored runtime evaluation
// as an assumption, which is not guaranteed and would make D2::f's proof of
// its contract_assert conditional, so never assumed; a label makes D2::f's
// precondition's runtime evaluation enforce, and still analyzes it with
// prove.)

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template <evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};
constexpr allow<enforce, prove> enforce_or_prove{};

int sink(int);

struct B1 { virtual int f(int x) pre<std::contracts::labels::always_enforce>(x > 0); };
struct D1 : B1 {
  virtual void key();
  int f(int x) override         // expected-remark {{override shadow: proven, sound}}
    pre(x >= 1)
  { return x >= 1 ? x : sink(x); }
};

struct B2 { virtual int f(int x) pre(x > 0); };
struct D2 : B2 {
  virtual void key();
  int f(int x) override         // expected-remark {{override shadow: proven, conditional}}
    pre<enforce_or_prove>(x >= 1)
  {
    if (x < 5)
      return 0;
    contract_assert(x >= 5);    // expected-remark {{proof shadow: proven, sound; assumed}}
    return x;
  }
};

// The key functions emit the vtables, and with them D1::f and D2::f.
void D1::key() {}
void D2::key() {}

// CHECK-LABEL: define linkonce_odr {{.*}}i32 @_ZN2D11fEi(
// CHECK: call {{.*}}i32 @_Z4sinki(
// CHECK: ret i32
// CHECK-NOT: contract.override
