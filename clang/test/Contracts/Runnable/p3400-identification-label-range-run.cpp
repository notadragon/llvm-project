// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=ignore \
// RUN:   -fcontract-configuration-file=%S/p3400-identification-label-range-run.json \
// RUN:   %libcxx_flags -o %t && %t

// P3400 [support.contract.control.ident]: a configuration file selects by
// group for every form of group_names -- any const range whose elements
// convert to string_view -- and for combined labels, whose group_names is the
// sorted, unique union of their constituents' (CLANG-826).  Only the "safety"
// group is observed; everything else is ignored, so the handler runs exactly
// for the labels whose names, read in full, include "safety".
//
// Constant-evaluation counterpart: ../p3400-identification-label-range.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range-run.C

#include <array>
#include <contracts>
#include <initializer_list>
#include <span>
#include <string_view>

namespace lbl = std::contracts::labels;
using lbl::operator|;
using lbl::operator""group;

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

struct ptr_long_t {
  using assertion_control_object = ptr_long_t;
  static constexpr const char *const group_names[3] = {
      "a", "a_group_name_much_longer_than_a_pointer", "safety"};
};

// "safety" is the long name here, and the short ones come after it.
struct ptr_short_t {
  using assertion_control_object = ptr_short_t;
  static constexpr const char *const group_names[3] = {"safety", "b", "c"};
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

constexpr const char *span_names[4] = {"w", "x", "y", "safety"};
struct span_t {
  using assertion_control_object = span_t;
  static constexpr std::span<const char *const> group_names{span_names};
};

constexpr std::string_view span_names_unsafe[3] = {"x", "y", "z"};
struct span_unsafe_t {
  using assertion_control_object = span_unsafe_t;
  static constexpr std::span<const std::string_view> group_names{
      span_names_unsafe};
};

void f_long(int x) pre<ptr_long_t{}>(x > 0) {}
void f_short(int x) pre<ptr_short_t{}>(x > 0) {}
void f_std_array(int x) pre<std_array_sv_t{}>(x > 0) {}
void f_sv_array(int x) pre<sv_array_t{}>(x > 0) {}
void f_init_list(int x) pre<init_list_t{}>(x > 0) {}
void f_span(int x) pre<span_t{}>(x > 0) {}
void f_unsafe(int x) pre<span_unsafe_t{}>(x > 0) {}

// Combined labels.
constexpr auto unsafe_span = span_unsafe_t{} | span_t{};
constexpr auto group_il = "q"group | init_list_t{};
constexpr auto nested =
    (span_unsafe_t{} | "w"group) | (lbl::review | sv_array_t{});
constexpr auto unsafe_only = span_unsafe_t{} | "w"group | span_unsafe_t{};
void f_unsafe_span(int x) pre<unsafe_span>(x > 0) {}
void f_group_il(int x) pre<group_il>(x > 0) {}
void f_nested(int x) pre<nested>(x > 0) {}
void f_unsafe_only(int x) pre<unsafe_only>(x > 0) {}
void f_inline(int x) pre<(ptr_short_t{} | std_array_sv_t{})>(x > 0) {}

// Walk a label's names at run time.
template <typename R>
bool names_are(const R &r, std::initializer_list<std::string_view> want) {
  auto w = want.begin();
  for (std::string_view n : r) {
    if (w == want.end() || *w != n)
      return false;
    ++w;
  }
  return w == want.end();
}

#define EXPECT(call, n)                                                        \
  do {                                                                         \
    reported = 0;                                                              \
    call;                                                                      \
    if (reported != (n))                                                       \
      __builtin_abort();                                                       \
  } while (0)

int main() {
  EXPECT(f_long(-1), 1);
  EXPECT(f_short(-1), 1);
  EXPECT(f_std_array(-1), 1);
  EXPECT(f_sv_array(-1), 1);
  EXPECT(f_init_list(-1), 1);
  EXPECT(f_span(-1), 1);
  EXPECT(f_unsafe(-1), 0);

  EXPECT(f_unsafe_span(-1), 1);
  EXPECT(f_group_il(-1), 1);
  EXPECT(f_nested(-1), 1);
  EXPECT(f_unsafe_only(-1), 0);
  EXPECT(f_inline(-1), 1);

  // A copy made at run time presents the same sorted, unique union.
  auto copy = nested;
  if (!names_are(copy.group_names, {"safety", "w", "x", "y", "z"}))
    __builtin_abort();
  auto bigger = copy | ptr_long_t{} | ptr_short_t{};
  if (!names_are(bigger.group_names,
                 {"a", "a_group_name_much_longer_than_a_pointer", "b", "c",
                  "safety", "w", "x", "y", "z"}))
    __builtin_abort();
  if (!names_are(unsafe_only.group_names, {"w", "x", "y", "z"}))
    __builtin_abort();
}
