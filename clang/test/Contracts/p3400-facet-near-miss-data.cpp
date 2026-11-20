// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -Wno-unused-command-line-argument %libcxx_flags -fsyntax-only 2>&1 \
// RUN:   | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -Wno-contract-invalid-label-facet -Wno-unused-command-line-argument \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 \
// RUN:   | FileCheck --check-prefix=QUIET --allow-empty %s

// P3400: the near-miss facet warning for a member that is both inaccessible
// and non-const, and for the data-member facets allowed_semantics and
// group_names (notadragon_wg21 DECISIONS-resolved.md E61).  A data member is
// reported when it is inaccessible, not const, or both, and its type still
// has the facet's shape: convertible to evaluation_semantic_set for
// allowed_semantics, indexable to something convertible to const char * for
// group_names.  A same-named member of another shape, or a member function
// of that name, stays silent, as p3400-facet-near-miss.cpp requires for the
// function facets.  The QUIET run is GCC's p3400-facet-near-miss-quiet.C for
// these shapes.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-near-miss-data.C
// (and the new shapes of p3400-facet-near-miss-quiet.C) in the gnu_gcc fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// A function facet that fails both ways at once.
struct both_comment_t {
  using assertion_control_object = both_comment_t;

private:
  constexpr const char *compute_comment(const char *) { return "x"; }
};
constexpr both_comment_t both_comment{};
// CHECK: warning: assertion-control object 'both_comment_t' does not provide the compute_comment facet because that member is inaccessible and not 'const'
// CHECK: note: declared here
void f1(int x) pre<both_comment>(x > 0) {}

// allowed_semantics: non-const (a static member without constexpr or const),
// inaccessible, and both.
struct mut_allowed_t {
  using assertion_control_object = mut_allowed_t;
  static inline evaluation_semantic_set allowed_semantics = {
      evaluation_semantic::ignore};
};
constexpr mut_allowed_t mut_allowed{};
// CHECK: warning: assertion-control object 'mut_allowed_t' does not provide the allowed_semantics facet because that member is not 'const'
// CHECK: note: declared here
void f2(int x) pre<mut_allowed>(x > 0) {}

struct priv_allowed_t {
  using assertion_control_object = priv_allowed_t;

private:
  static constexpr evaluation_semantic allowed_semantics =
      evaluation_semantic::ignore;
};
constexpr priv_allowed_t priv_allowed{};
// CHECK: warning: assertion-control object 'priv_allowed_t' does not provide the allowed_semantics facet because that member is inaccessible
// CHECK-NOT: and not 'const'
// CHECK: note: declared here
void f3(int x) pre<priv_allowed>(x > 0) {}

class both_allowed_t {
public:
  using assertion_control_object = both_allowed_t;

private:
  evaluation_semantic_set allowed_semantics = {};
};
constexpr both_allowed_t both_allowed{};
// CHECK: warning: assertion-control object 'both_allowed_t' does not provide the allowed_semantics facet because that member is inaccessible and not 'const'
// CHECK: note: declared here
void f4(int x) pre<both_allowed>(x > 0) {}

// group_names: non-const, inaccessible.
struct mut_group_t {
  using assertion_control_object = mut_group_t;
  static inline const char *group_names[] = {"g"};
};
constexpr mut_group_t mut_group{};
// CHECK: warning: assertion-control object 'mut_group_t' does not provide the group_names facet because that member is not 'const'
// CHECK: note: declared here
void f5(int x) pre<mut_group>(x > 0) {}

struct priv_group_t {
  using assertion_control_object = priv_group_t;

private:
  static constexpr char group_names[][2] = {"g"};
};
constexpr priv_group_t priv_group{};
// CHECK: warning: assertion-control object 'priv_group_t' does not provide the group_names facet because that member is inaccessible
// CHECK-NOT: and not 'const'
// CHECK: note: declared here
void f6(int x) pre<priv_group>(x > 0) {}

// Same names, other shapes or kinds: silent.
struct other_shapes_t {
  using assertion_control_object = other_shapes_t;
  int allowed_semantics = 0; // not convertible to evaluation_semantic_set
  int group_names = 0;       // not indexable
};
constexpr other_shapes_t other_shapes{};
void f7(int x) pre<other_shapes>(x > 0) {}

struct other_kinds_t {
  using assertion_control_object = other_kinds_t;

private:
  int allowed_semantics() const { return 0; }
  struct group_names {};
};
constexpr other_kinds_t other_kinds{};
void f8(int x) pre<other_kinds>(x > 0) {}

// CHECK-NOT: warning:
// CHECK-NOT: error:

// QUIET-NOT: warning:
// QUIET-NOT: error:

// REQUIRES: contracts-libcxx
