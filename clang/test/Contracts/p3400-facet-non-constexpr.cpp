// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

// P3400: a facet consumed during translation must be usable in a constant
// expression, and failing to be is a hard error rather than a silent no-op.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-non-constexpr.C)
//
// No concept can ask whether a member is constexpr, so per D3400R5 a type with
// the right member *does* participate in the facet, and the error arrives when
// the result is evaluated during translation.  Previously the evaluation was
// attempted with diagnostics discarded and the facet quietly did nothing --
// the label looked applied, and the comment or semantic was simply unchanged.
// That is the quietest way to get a label wrong: no diagnostic, no effect.
//
// compute_semantic, compute_comment and compute_message are the
// translation-time facets.  handle_contract_violation and query run at run time
// and are unaffected; allowed_semantics and group_names are data members of a
// constant object and so are already constrained.

#include <contracts>

using std::contracts::evaluation_semantic;

struct non_constexpr_semantic_t {
  using assertion_control_object = non_constexpr_semantic_t;
  evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
constexpr non_constexpr_semantic_t non_constexpr_semantic{};

struct non_constexpr_comment_t {
  using assertion_control_object = non_constexpr_comment_t;
  const char *compute_comment(const char *) const { return "x"; }
};
constexpr non_constexpr_comment_t non_constexpr_comment{};

struct non_constexpr_message_t {
  using assertion_control_object = non_constexpr_message_t;
  const char *compute_message(const char *) const { return "x"; }
};
constexpr non_constexpr_message_t non_constexpr_message{};

// The member satisfies the concept, so the facet is present; the error comes
// from evaluating it.
// CHECK: error: compute_semantic facet of assertion-control object 'non_constexpr_semantic_t' must be usable in a constant expression
void f1(int x) pre<non_constexpr_semantic>(x > 0) {}
// CHECK: error: compute_comment facet of assertion-control object 'non_constexpr_comment_t' must be usable in a constant expression
void f2(int x) pre<non_constexpr_comment>(x > 0) {}
// CHECK: error: compute_message facet of assertion-control object 'non_constexpr_message_t' must be usable in a constant expression
void f3(int x) pre<non_constexpr_message>(x > 0) {}

// The same shapes with constexpr members are accepted, so the checks above are
// pinning constexpr-ness and not some unrelated property of the label.
struct ok_semantic_t {
  using assertion_control_object = ok_semantic_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic s) const {
    return s;
  }
};
struct ok_comment_t {
  using assertion_control_object = ok_comment_t;
  constexpr const char *compute_comment(const char *) const { return "x"; }
};
struct ok_message_t {
  using assertion_control_object = ok_message_t;
  constexpr const char *compute_message(const char *) const { return "x"; }
};
constexpr ok_semantic_t ok_semantic{};
constexpr ok_comment_t ok_comment{};
constexpr ok_message_t ok_message{};

void g1(int x) pre<ok_semantic>(x > 0) {}
void g2(int x) pre<ok_comment>(x > 0) {}
void g3(int x) pre<ok_message>(x > 0) {}

// A run-time facet is unaffected: a non-constexpr handler is perfectly fine.
struct runtime_handler_t {
  using assertion_control_object = runtime_handler_t;
  void handle_contract_violation(
      const std::contracts::contract_violation &) const {}
};
constexpr runtime_handler_t runtime_handler{};

void h(int x) pre<runtime_handler>(x > 0) {}

// CHECK-NOT: error:
