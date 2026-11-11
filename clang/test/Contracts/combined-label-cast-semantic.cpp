// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// A combined label applies each operand's compute_semantic result by
// implicit conversion to evaluation_semantic, the conversion the concept
// checks (convertible_to; CLANG-828).  An operand whose facet returns int, or
// a type only explicitly convertible to evaluation_semantic, is not a
// semantic_computation_label: combined with review, only review's
// compute_semantic applies, and combined with a label that has none, the
// result has none either.  An operand whose result converts implicitly
// combines.  Each operand facet maps every semantic to ignore, and review
// maps enforce to observe and leaves ignore alone.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/combined-label-cast-semantic.C

#include <contracts>

using std::contracts::evaluation_semantic;
namespace L = std::contracts::labels;

struct int_sem_t {
  using assertion_control_object = int_sem_t;
  constexpr int compute_semantic(evaluation_semantic) const { return 1; }
};
constexpr int_sem_t int_sem{};

struct Sem {
  constexpr explicit operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct expl_sem_t {
  using assertion_control_object = expl_sem_t;
  constexpr Sem compute_semantic(evaluation_semantic) const { return {}; }
};
constexpr expl_sem_t expl_sem{};

struct ImplSem {
  constexpr operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct impl_sem_t {
  using assertion_control_object = impl_sem_t;
  constexpr ImplSem compute_semantic(evaluation_semantic) const { return {}; }
};
constexpr impl_sem_t impl_sem{};

static_assert(!L::semantic_computation_label<int_sem_t>);
static_assert(!L::semantic_computation_label<expl_sem_t>);
static_assert(L::semantic_computation_label<impl_sem_t>);

constexpr auto c1 = int_sem | L::review;
constexpr auto c2 = expl_sem | L::review;
constexpr auto c3 = impl_sem | L::review;
static_assert(c1.compute_semantic(evaluation_semantic::enforce) ==
              evaluation_semantic::observe);
static_assert(c2.compute_semantic(evaluation_semantic::enforce) ==
              evaluation_semantic::observe);
static_assert(c3.compute_semantic(evaluation_semantic::enforce) ==
              evaluation_semantic::ignore);

constexpr auto d1 = int_sem | L::always_observe;
constexpr auto d2 = expl_sem | L::always_observe;
constexpr auto d3 = impl_sem | L::always_observe;
static_assert(!L::semantic_computation_label<decltype(d1)>);
static_assert(!L::semantic_computation_label<decltype(d2)>);
static_assert(L::semantic_computation_label<decltype(d3)>);

// REQUIRES: contracts-libcxx
