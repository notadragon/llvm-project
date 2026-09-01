// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: allowed_semantics must be const, exactly as group_names must be.
//
// D3400R5 requires is_const_v<decltype(t.allowed_semantics)>, so a
// `static constexpr' member qualifies and so does a plain `const' one, but a
// non-const member is not a facet at all -- and a front end that reads it
// anyway narrows the semantic set for a label the library says has no
// allowed_semantics facet, so the bare label and its combined form disagree.
//
// The rule was implemented on both compilers without a negative test.  Its
// sibling for group_names got one (Runnable/p3400-facet-group-const.cpp);
// this one was only mentioned in a comment.  So nothing pinned the case that
// actually changed behaviour: a non-const allowed_semantics used to be
// honoured and is silently ignored now -- silently, because the near-miss
// warning deliberately does not probe the data-member facets.
//
// GCC mirror: g++.dg/contracts/cpp26/p3400-facet-allowed-const.C.

#include <contracts>
#include <cstdio>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
namespace lbl = std::contracts::labels;
using lbl::operator|;

static int reported = 0;
static int failures = 0;

void handle_contract_violation(const contract_violation &) { ++reported; }

static void check(const char *what, int want) {
  if (reported != want) {
    std::printf("FAIL: %s: expected %d, got %d\n", what, want, reported);
    ++failures;
  }
  reported = 0;
}

// Restricting to {ignore} means a violation must NOT be reported.

// The static spelling: a facet.
struct static_allowed_t {
  using assertion_control_object = static_allowed_t;
  static constexpr evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set{evaluation_semantic::ignore};
};
constexpr static_allowed_t static_allowed{};

// The const-non-static spelling: also a facet.
struct const_allowed_t {
  using assertion_control_object = const_allowed_t;
  const evaluation_semantic_set allowed_semantics;
  constexpr const_allowed_t(evaluation_semantic_set s)
      : allowed_semantics(s) {}
};
constexpr const_allowed_t const_allowed{
    evaluation_semantic_set{evaluation_semantic::ignore}};

// Non-const: not a facet, so the semantic is NOT narrowed and the
// command-line `observe' stands.
struct mutable_allowed_t {
  using assertion_control_object = mutable_allowed_t;
  evaluation_semantic_set allowed_semantics =
      evaluation_semantic_set{evaluation_semantic::ignore};
};
constexpr mutable_allowed_t mutable_allowed{};

static_assert(lbl::allowed_semantics_label<static_allowed_t>);
static_assert(lbl::allowed_semantics_label<const_allowed_t>);
static_assert(!lbl::allowed_semantics_label<mutable_allowed_t>);

// Combining must reach the same verdict as the bare label, in either order --
// that is what the front end and the concepts disagreeing would break.
constexpr auto static_combined = static_allowed | lbl::empty_label;
constexpr auto const_combined = const_allowed | lbl::empty_label;
constexpr auto mutable_combined = mutable_allowed | lbl::empty_label;
constexpr auto mutable_combined2 = lbl::empty_label | mutable_allowed;

static_assert(lbl::allowed_semantics_label<decltype(static_combined)>);
static_assert(lbl::allowed_semantics_label<decltype(const_combined)>);
static_assert(!lbl::allowed_semantics_label<decltype(mutable_combined)>);
static_assert(!lbl::allowed_semantics_label<decltype(mutable_combined2)>);

void f_static(int x) pre<static_allowed>(x > 0) {}
void f_const(int x) pre<const_allowed>(x > 0) {}
void f_mutable(int x) pre<mutable_allowed>(x > 0) {}
void f_static_c(int x) pre<static_combined>(x > 0) {}
void f_const_c(int x) pre<const_combined>(x > 0) {}
void f_mut_c(int x) pre<mutable_combined>(x > 0) {}
void f_mut_c2(int x) pre<mutable_combined2>(x > 0) {}

int main() {
  reported = 0;

  f_static(-1);
  check("static allowed_semantics restricts to ignore", 0);
  f_const(-1);
  check("const allowed_semantics restricts to ignore", 0);

  // The case with no test before this one.
  f_mutable(-1);
  check("non-const allowed_semantics is not a facet", 1);

  f_static_c(-1);
  check("combined static restricts", 0);
  f_const_c(-1);
  check("combined const restricts", 0);
  f_mut_c(-1);
  check("combined non-const does not restrict", 1);
  f_mut_c2(-1);
  check("combined non-const does not restrict (rhs)", 1);

  return failures;
}
