// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.ident]: group_names is any const range
// whose elements convert to string_view (CLANG-826).  The library concept,
// the combined label's union and Sema's reading of the names all walk the
// range, so neither the extent nor the element type of an array matters, and
// a range whose type does not say how many names it holds -- a span, an
// initializer_list -- is read in full.
//
// Each label puts "safety" last, and the safety group is ignored, so a
// violated precondition of a label whose groups are read is not evaluated
// during constant evaluation; one whose groups are missed is enforced and
// makes the call non-constant.
//
// Run-time counterpart: Runnable/p3400-identification-label-range-run.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range.C

#include <array>
#include <contracts>
#include <initializer_list>
#include <span>
#include <string_view>

namespace lbl = std::contracts::labels;
using lbl::identification_label;
using lbl::operator|;
using lbl::operator""group;

struct ptr_array_t {
  using assertion_control_object = ptr_array_t;
  static constexpr const char *const group_names[2] = {"x", "safety"};
};

// Names shorter and longer than a pointer.
struct ptr_long_t {
  using assertion_control_object = ptr_long_t;
  static constexpr const char *const group_names[3] = {
      "a", "a_group_name_much_longer_than_a_pointer", "safety"};
};

struct std_array_sv_t {
  using assertion_control_object = std_array_sv_t;
  static constexpr std::array<std::string_view, 2> group_names = {"x",
                                                                  "safety"};
};

struct sv_array_t {
  using assertion_control_object = sv_array_t;
  static constexpr std::string_view group_names[2] = {"x", "safety"};
};

struct init_list_t {
  using assertion_control_object = init_list_t;
  static constexpr std::initializer_list<const char *> group_names = {
      "x", "safety"};
};

constexpr const char *span_names[3] = {"x", "y", "safety"};
struct span_t {
  using assertion_control_object = span_t;
  static constexpr std::span<const char *const> group_names{span_names};
};

// The same names without "safety": a label whose groups are read in full is
// still enforced.
constexpr const char *span_names_unsafe[3] = {"x", "y", "z"};
struct span_unsafe_t {
  using assertion_control_object = span_unsafe_t;
  static constexpr std::span<const char *const> group_names{span_names_unsafe};
};

static_assert(identification_label<ptr_array_t>);
static_assert(identification_label<ptr_long_t>);
static_assert(identification_label<std_array_sv_t>);
static_assert(identification_label<sv_array_t>);
static_assert(identification_label<init_list_t>);
static_assert(identification_label<span_t>);

// A range of something that does not convert to string_view is not one.
struct int_array_t {
  using assertion_control_object = int_array_t;
  static constexpr int group_names[2] = {1, 2};
};
static_assert(!identification_label<int_array_t>);

constexpr int a(int x) pre<ptr_array_t{}>(x > 0) { return x; }
constexpr int b(int x) pre<std_array_sv_t{}>(x > 0) { return x; }
constexpr int c(int x) pre<sv_array_t{}>(x > 0) { return x; }
constexpr int d(int x) pre<init_list_t{}>(x > 0) { return x; }
constexpr int e(int x) pre<span_t{}>(x > 0) { return x; }
constexpr int f(int x) pre<ptr_long_t{}>(x > 0) { return x; }

static_assert(a(-1) == -1);
static_assert(b(-1) == -1);
static_assert(c(-1) == -1);
static_assert(d(-1) == -1);
static_assert(e(-1) == -1);
static_assert(f(-1) == -1);

// Combined labels: the union is read in full whatever the constituents'
// range types, and the safety group survives combining.
constexpr auto span_ptr = span_unsafe_t{} | ptr_long_t{};
constexpr auto il_group = init_list_t{} | "q"group;
constexpr auto sv_arr = sv_array_t{} | std_array_sv_t{};
constexpr auto nested =
    (span_unsafe_t{} | "w"group) | (lbl::review | span_t{});
constexpr auto one_side = lbl::review | span_unsafe_t{};
static_assert(identification_label<decltype(span_ptr)>);
static_assert(identification_label<decltype(il_group)>);
static_assert(identification_label<decltype(sv_arr)>);
static_assert(identification_label<decltype(nested)>);
static_assert(identification_label<decltype(one_side)>);
static_assert(!identification_label<decltype(lbl::review | lbl::review)>);

constexpr int g(int x) pre<span_ptr>(x > 0) { return x; }
constexpr int h(int x) pre<il_group>(x > 0) { return x; }
constexpr int i(int x) pre<sv_arr>(x > 0) { return x; }
constexpr int j(int x) pre<nested>(x > 0) { return x; }
constexpr int k(int x) pre<(span_unsafe_t{} | span_t{})>(x > 0) { return x; }

static_assert(g(-1) == -1);
static_assert(h(-1) == -1);
static_assert(i(-1) == -1);
static_assert(j(-1) == -1);
static_assert(k(-1) == -1);

// Control: no constituent names "safety", so the precondition is enforced.
constexpr auto unsafe = span_unsafe_t{} | "w"group;
constexpr int u(int x) pre<unsafe>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
static_assert(u(-1) == -1); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// The union is sorted and unique.
template <typename R>
constexpr bool names_are(const R &r,
                         std::initializer_list<std::string_view> want) {
  auto w = want.begin();
  for (std::string_view n : r) {
    if (w == want.end() || *w != n)
      return false;
    ++w;
  }
  return w == want.end();
}

static_assert(names_are(span_ptr.group_names,
                        {"a", "a_group_name_much_longer_than_a_pointer",
                         "safety", "x", "y", "z"}));
static_assert(names_are(il_group.group_names, {"q", "safety", "x"}));
static_assert(names_are(sv_arr.group_names, {"safety", "x"}));
static_assert(names_are(nested.group_names, {"safety", "w", "x", "y", "z"}));
static_assert(names_are(one_side.group_names, {"x", "y", "z"}));
static_assert(names_are(("b"group | "a"group | "b"group).group_names,
                        {"a", "b"}));
