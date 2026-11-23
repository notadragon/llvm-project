// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore -fsyntax-only \
// RUN:   -verify %s
// RUN: not %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore -fsyntax-only \
// RUN:   -DSTRING_VIEW %s 2>&1 | FileCheck %s --check-prefix=SV
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E189 (a): a label whose group_names is a range of names, but whose reading
// is not a constant expression, is an error, followed by the evaluator's
// account of why: silently giving it no groups would configure the
// assertion as unlabelled (here: enforced rather than ignored).  A
// group_names that is not such a range stays undiagnosed, as an absent
// one does: the label simply has no groups.  Without <string_view> in scope
// the names are read as const char*; the second run reads them as
// string_view.  The step limit is p3400-group-names-steps-limit.cpp, and
// Clang's cap on the number of names is p3400-group-names-too-many.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-not-constant.C

#ifdef STRING_VIEW
#include <string_view>
#endif

constexpr const char *names[2] = {"a", "safety"};  // expected-note {{array 'names' declared here}}

// begin() has undefined behaviour: it reads past the end of names.
struct ub_range {
  constexpr const char *const *begin() const
  { return names + (names[2] != nullptr); }  // expected-warning {{array index 2 is past the end of the array}} \
                                             // expected-note {{read of dereferenced one-past-the-end pointer is not allowed in a constant expression}}
  constexpr const char *const *end() const { return names + 2; }
};

struct ub_t {
  using assertion_control_object = ub_t;
  static constexpr ub_range group_names{};
};
constexpr ub_t ub{};

// A name that is not a constant: the array is not constexpr.
extern const char *const runtime_names[1];  // expected-note {{declared here}}

struct runtime_range {
  constexpr const char *const *begin() const { return runtime_names; }
  constexpr const char *const *end() const { return runtime_names + 1; }
};

struct runtime_t {
  using assertion_control_object = runtime_t;
  static constexpr runtime_range group_names{};
};
constexpr runtime_t runtime_name{};

// Not a range of names: no groups, no diagnostic.
struct ints_t {
  using assertion_control_object = ints_t;
  static constexpr int group_names[2] = {1, 2};
};
constexpr ints_t ints{};

struct scalar_t {
  using assertion_control_object = scalar_t;
  static constexpr int group_names = 3;
};
constexpr scalar_t scalar{};

// A good one, for contrast.
struct good_t {
  using assertion_control_object = good_t;
  static constexpr const char *group_names[2] = {"a", "safety"};
};
constexpr good_t good{};

void f1(int x) pre<ub>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'ub_t' must be usable in a constant expression}} \
                                  // expected-note {{in call to}}
void f2(int x) pre<runtime_name>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'runtime_t' must be usable in a constant expression}} \
                                            // expected-note {{read of non-constexpr variable 'runtime_names' is not allowed in a constant expression}}
void f3(int x) pre<ints>(x > 0) {}
void f4(int x) pre<scalar>(x > 0) {}
void f5(int x) pre<good>(x > 0) {}

// SV: error: group_names facet of assertion-control object 'ub_t' must be usable in a constant expression
// SV: error: group_names facet of assertion-control object 'runtime_t' must be usable in a constant expression
// SV-NOT: error:
