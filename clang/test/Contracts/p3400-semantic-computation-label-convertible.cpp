// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fsyntax-only %s
// REQUIRES: contracts-libcxx

// D3400R5 [support.contract.control.compute] requires
// { t.compute_semantic(s) } -> convertible_to<evaluation_semantic>, an
// implicit conversion, as the core definition of a computed-semantics
// control type does ([basic.contract.control]; CLANG-828).  So a label whose
// compute_semantic returns int, or a class with only an explicit conversion
// to evaluation_semantic, is not a semantic-computation label, and one whose
// result is evaluation_semantic is.  The Sema half is
// Runnable/p3400-compute-semantic-int-result.cpp and
// p3400-compute-semantic-convertible.cpp.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-compute-semantic-convertible.C

#include <contracts>
#include <type_traits>

using std::contracts::evaluation_semantic;
namespace L = std::contracts::labels;

struct int_sem_t {
  using assertion_control_object = int_sem_t;
  constexpr int compute_semantic(evaluation_semantic) const {
    return int(evaluation_semantic::observe);
  }
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

struct sem_t {
  using assertion_control_object = sem_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
constexpr sem_t sem{};

static_assert(!std::is_convertible_v<int, evaluation_semantic>);
static_assert(!std::is_convertible_v<Sem, evaluation_semantic>);

static_assert(L::semantic_computation_label<sem_t>);
static_assert(L::is_semantic_computation_label<sem>);

static_assert(!L::semantic_computation_label<int_sem_t>);
static_assert(!L::is_semantic_computation_label<int_sem>);
static_assert(!L::semantic_computation_label<expl_sem_t>);
static_assert(!L::is_semantic_computation_label<expl_sem>);
