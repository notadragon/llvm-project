// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// A combined label applies each operand's compute_semantic result through
// the functional cast the concept checks (evaluation_semantic(...)), so an
// operand whose facet returns int, or a type explicitly convertible to
// evaluation_semantic, combines.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/combined-label-cast-semantic.C

#include <contracts>

using std::contracts::evaluation_semantic;
namespace L = std::contracts::labels;

struct int_sem_t {
  using assertion_control_object = int_sem_t;
  constexpr int compute_semantic(evaluation_semantic) const { return 2; }
};
constexpr int_sem_t int_sem{};

struct Sem {
  constexpr explicit operator evaluation_semantic() const {
    return evaluation_semantic::observe;
  }
};
struct expl_sem_t {
  using assertion_control_object = expl_sem_t;
  constexpr Sem compute_semantic(evaluation_semantic) const { return {}; }
};
constexpr expl_sem_t expl_sem{};

static_assert(L::semantic_computation_label<int_sem_t>);
static_assert(L::semantic_computation_label<expl_sem_t>);

constexpr auto c1 = int_sem | L::review;
constexpr auto c2 = expl_sem | L::review;
constexpr evaluation_semantic r1 =
    c1.compute_semantic(evaluation_semantic::enforce);
constexpr evaluation_semantic r2 =
    c2.compute_semantic(evaluation_semantic::enforce);

// REQUIRES: contracts-libcxx
