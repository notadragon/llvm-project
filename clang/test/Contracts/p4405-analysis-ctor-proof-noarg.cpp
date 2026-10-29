// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2 %libcxx_flags -S -emit-llvm -o /dev/null -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405 ("The Analysis Evaluation"): a precondition's analysis evaluation
// at a call with no argument but the object -- a default constructor, a
// destructor, a function returning a class with no parameters -- is
// deferred to the proof shadow like any other, so a guarded call is proven
// when optimizing.  A destructor called implicitly (an automatic variable
// going out of scope, a delete-expression) is diagnosed at the variable or
// the delete-expression.  The same precondition on a function returning int
// is proven too, and unguarded calls still cannot be.  A [[nodiscard]] call
// is still found to be discarded.  An unproven evaluation is a premise for
// those after it, so the unguarded calls are in functions of their own.
// (H's constructor is defined out of line: Clang does not diagnose an
// inline function it never emits, DECISIONS.md E130.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-ctor-proof-noarg.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

extern int g;
// A call (operator new, another element's constructor) may change g, so
// the calls that follow one in a new-expression, an array initialization
// or a delete-expression have preconditions on k, which no call changes.
extern const int k;

struct T {
  T() pre<always_prove>(g > 0); // expected-note 3 {{the predicate of the contract assertion}}
  int v = 0;
};
struct D {
  ~D() pre<always_prove>(g > 0); // expected-note {{the predicate of the contract assertion}}
};
struct R { int v; };
R mk() pre<always_prove>(g > 0); // expected-note {{the predicate of the contract assertion}}
int mi() pre<always_prove>(g > 0); // expected-note {{the predicate of the contract assertion}}
struct M {
  R get() const pre<always_prove>(g > 0); // expected-note {{the predicate of the contract assertion}}
};
R operator+(R, R) pre<always_prove>(g > 0); // expected-note {{the predicate of the contract assertion}}
[[nodiscard]] R nd() pre<always_prove>(g > 0);
struct TK {
  TK() pre<always_prove>(k > 0); // expected-note {{the predicate of the contract assertion}}
  int v = 0;
};
struct DK {
  ~DK() pre<always_prove>(k > 0); // expected-note {{the predicate of the contract assertion}}
};
struct H {
  T t;
  H(int);
};
H::H(int) : t() {} // expected-error {{contract assertion cannot be proven}}

T ret() {
  if (g > 0)
    return T();
  __builtin_unreachable();
}

int use(M &m, R a, R b) {
  int r = 0;
  if (g > 0)
    T t;
  if (g > 0) {
    D d;
  }
  if (g > 0)
    r += mk().v;
  if (g > 0)
    r += mi();
  if (g > 0)
    r += T().v;
  if (g > 0)
    r += T{}.v;
  if (k > 0)
    r += (new TK)->v;
  if (g > 0)
    r += m.get().v;
  if (g > 0)
    r += (a + b).v;
  if (k > 0) {
    TK arr[2];
    r += arr[1].v;
  }
  if (g > 0) {
    R x = mk();
    r += x.v;
  }
  if (g > 0) {
    T y = T();
    r += y.v;
  }
  if (k > 0)
    delete new DK;
  if (g > 0)
    nd(); // expected-warning {{ignoring return value of function declared with 'nodiscard' attribute}}
  return r;
}

void u1() { T t; }                   // expected-error {{contract assertion cannot be proven}}
void u2() { D d; }                   // expected-error {{contract assertion cannot be proven}}
int u3() { return mk().v; }          // expected-error {{contract assertion cannot be proven}}
int u4() { return T().v; }           // expected-error {{contract assertion cannot be proven}}
int u5(M &m) { return m.get().v; }   // expected-error {{contract assertion cannot be proven}}
int u6(R a, R b) { return (a + b).v; } // expected-error {{contract assertion cannot be proven}}
int u7() { return mi(); }            // expected-error {{contract assertion cannot be proven}}
int u8() { return (new TK)->v; }     // expected-error {{contract assertion cannot be proven}}
void u9() { delete new DK; }         // expected-error {{contract assertion cannot be proven}}
