// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-evaluation-semantic=ignore %libcxx_flags -fsyntax-only -Xclang -verify -Xclang -verify-ignore-unexpected=note %s

// D4405 ("P3400R5 Analysis Evaluation Configuration"): an analysis_semantic
// facet must be a constant expression whose value is ignore, diagnose or
// prove; otherwise the program is ill-formed.  A combined label passes an
// invalid value on.  A member that does not model analysis_semantic_label
// (here, one taking an argument) is no facet, and is ignored.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-facet-invalid.C

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using enum evaluation_semantic;

template <evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic() const { return S; }
};

struct not_constexpr {
  using assertion_control_object = not_constexpr;
  evaluation_semantic analysis_semantic() const { return prove; }
};

struct takes_argument {
  using assertion_control_object = takes_argument;
  constexpr evaluation_semantic analysis_semantic(evaluation_semantic s) const {
    return s;
  }
};

constexpr analyzed<enforce> to_enforce{};
constexpr analyzed<unspecified> to_unspecified{};
constexpr analyzed<(evaluation_semantic)10> to_unknown{};
constexpr analyzed<diagnose> diagnosed{};
constexpr not_constexpr nc{};
constexpr takes_argument ta{};

int a(int x) pre<to_enforce>(x > 0) { return x; }             // expected-error-re {{analysis_semantic result of assertion-control label of type 'analyzed<{{.*}}>' is not 'ignore', 'diagnose' or 'prove'}}
int b(int x) pre<to_unspecified>(x > 0) { return x; }         // expected-error {{is not 'ignore', 'diagnose' or 'prove'}}
int c(int x) pre<to_unknown>(x > 0) { return x; }             // expected-error {{is not 'ignore', 'diagnose' or 'prove'}}
int d(int x) pre<diagnosed | to_enforce>(x > 0) { return x; } // expected-error {{is not 'ignore', 'diagnose' or 'prove'}}
int e(int x) pre<nc>(x > 0) { return x; }                     // expected-error {{analysis_semantic facet of assertion-control object 'not_constexpr' must be usable in a constant expression}}
int f(int x) pre<ta>(x > 0) { return x; }

// REQUIRES: contracts-libcxx
