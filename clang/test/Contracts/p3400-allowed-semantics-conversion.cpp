// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s

// The allowed_semantics facet is evaluation_semantic_set
// ({label.allowed_semantics}) ([support.contract.control.allowed]), so the
// allowed semantics are asked of that set (DECISIONS.md K3).  A member with no
// conversion to evaluation_semantic_set is not the facet at all -- whatever
// contains () it has -- and the label is unrestricted.  A member whose
// conversion is not a constant expression is the facet, and is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-allowed-semantics-conversion.C
// in the gnu_gcc fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct only_observe_set {
  bool contains (evaluation_semantic s) const   // not constexpr
  { return s == evaluation_semantic::observe; }
};

struct not_a_facet_t {
  using assertion_control_object = not_a_facet_t;
  static constexpr only_observe_set allowed_semantics{};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};
constexpr not_a_facet_t not_a_facet{};

void f (int x) pre<not_a_facet> (x > 0) {}	// OK, unrestricted

struct converts_late {
  operator evaluation_semantic_set () const	// not constexpr
  { return evaluation_semantic::observe; }
};

struct late_facet_t {
  using assertion_control_object = late_facet_t;
  static constexpr converts_late allowed_semantics{};
};
constexpr late_facet_t late_facet{};

void g (int x) pre<late_facet> (x > 0) {}	// expected-error {{allowed_semantics facet of assertion-control object 'late_facet_t' must be usable in a constant expression}}

// REQUIRES: contracts-libcxx
