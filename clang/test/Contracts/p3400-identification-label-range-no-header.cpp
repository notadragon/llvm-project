// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore -fsyntax-only \
// RUN:   -verify %s

// P3400 [support.contract.control.ident]: Sema reads group_names as a range,
// as a range-based for would walk it -- begin and end as members, or found by
// argument-dependent lookup -- and converts each element to string_view
// (CLANG-826).  A label needs no header, and without <string_view> in scope
// the elements are read as const char*.
//
// Each label puts "safety" among its names, and the safety group is ignored,
// so a violated precondition is not evaluated during constant evaluation only
// when the names are read in full.  A group_names that is not const, or whose
// elements do not convert, names no groups.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range-no-header.C

constexpr const char *names[4] = {"a", "b", "c", "safety"};

// A range with member begin and end.
struct member_range {
  const char *const *b, *const *e;
  constexpr const char *const *begin() const { return b; }
  constexpr const char *const *end() const { return e; }
};

// A range whose begin and end are found by argument-dependent lookup, with a
// class iterator and a distinct sentinel type.
namespace adl {
struct sentinel {
  const char *const *p;
};
struct iterator {
  const char *const *p;
  constexpr const char *operator*() const { return *p; }
  constexpr iterator &operator++() {
    ++p;
    return *this;
  }
  friend constexpr bool operator==(const iterator &i, const sentinel &s) {
    return i.p == s.p;
  }
};
struct range {
  const char *const *b, *const *e;
};
constexpr iterator begin(const range &r) { return {r.b}; }
constexpr sentinel end(const range &r) { return {r.e}; }
} // namespace adl

// An element type that converts to const char*.
struct name {
  const char *s;
  constexpr operator const char *() const { return s; }
};

struct member_t {
  using assertion_control_object = member_t;
  static constexpr member_range group_names{names, names + 4};
};

struct adl_t {
  using assertion_control_object = adl_t;
  static constexpr adl::range group_names{names, names + 4};
};

struct name_array_t {
  using assertion_control_object = name_array_t;
  static constexpr name group_names[2] = {{"x"}, {"safety"}};
};

struct ptr_array_t {
  using assertion_control_object = ptr_array_t;
  const char *const group_names[3] = {"x", "a_name_longer_than_a_pointer",
                                      "safety"};
};

// Stops short of "safety": its groups are read, and do not include it.
struct short_t {
  using assertion_control_object = short_t;
  static constexpr member_range group_names{names, names + 3};
};

// Not const: no groups.
struct nonconst_t {
  using assertion_control_object = nonconst_t;
  const char *group_names[1] = {"safety"}; // expected-note {{declared here}}
};

// Elements that do not convert: no groups.
struct int_array_t {
  using assertion_control_object = int_array_t;
  static constexpr int group_names[2] = {1, 2};
};

constexpr int a(int x) pre<member_t{}>(x > 0) { return x; }
constexpr int b(int x) pre<adl_t{}>(x > 0) { return x; }
constexpr int c(int x) pre<name_array_t{}>(x > 0) { return x; }
constexpr int d(int x) pre<ptr_array_t{}>(x > 0) { return x; }
constexpr int e(int x) pre<short_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int f(int x) pre<nonconst_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}} expected-warning {{assertion-control object 'nonconst_t' does not provide the group_names facet because that member is not 'const'}}
constexpr int g(int x) pre<int_array_t{}>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

static_assert(a(-1) == -1);
static_assert(b(-1) == -1);
static_assert(c(-1) == -1);
static_assert(d(-1) == -1);
static_assert(e(-1) == -1); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
static_assert(f(-1) == -1); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
static_assert(g(-1) == -1); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
