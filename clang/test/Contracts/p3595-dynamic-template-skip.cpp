// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-template-skip.json -fsyntax-only -verify %s

// expected-no-diagnostics

// P3595 dynamic selection: population must be SKIPPED in a dependent
// (uninstantiated) template body.  Sema::BuildContractStmt guards
// populateContractSemanticState on a non-dependent context; while parsing a
// template body the label type / config resolution are not yet meaningful, so
// the dynamic descriptor and (for labels) the transform table are not computed.
//
// These templates are defined with dynamic-configured contracts but are NEVER
// instantiated, so population must not run at all.  If the guard regressed and
// population ran on the dependent body, it would attempt to resolve config /
// constant-evaluate label facets against dependent types -- which crashes or
// emits spurious diagnostics.  A clean -verify with expected-no-diagnostics
// locks in that the dependent body is safely skipped.
//
// The instantiation-side behavior (population DOES run on instantiation and
// dispatches dynamically) is covered by Runnable/p3595-dynamic-template.
//
// This is a -cc1 test (no standard include paths), so the label facet types are
// defined inline rather than pulled from <contracts>.

namespace std::contracts {
enum class evaluation_semantic : unsigned char {
  ignore = 1, observe, enforce, quick_enforce
};
} // namespace std::contracts

// A label object carrying a compute_semantic facet -- the sort of thing whose
// evaluation on a dependent type would misbehave if population were not skipped.
struct to_observe_t {
  using assertion_control_object = to_observe_t;
  constexpr std::contracts::evaluation_semantic
  compute_semantic(std::contracts::evaluation_semantic __s) const {
    using enum std::contracts::evaluation_semantic;
    if (__s == enforce || __s == quick_enforce)
      return observe;
    return __s;
  }
};
constexpr to_observe_t to_observe{};

// Unlabeled dynamic contract in a dependent body.
template <class T>
void f(const T x) pre(x > 0) {}

// Labeled dynamic contract in a dependent body: applyLabelFacets (and its
// dynamic-table precompute) must be skipped while T is dependent.
template <class T>
void g(const T x) pre<to_observe>(x > 0) {}

// contract_assert inside a dependent body, unlabeled and labeled.
template <class T>
void h(const T x) {
  contract_assert(x > 0);
  contract_assert<to_observe>(x > 0);
}

// Nothing is instantiated: no f<...>, g<...>, or h<...> is ever formed.
