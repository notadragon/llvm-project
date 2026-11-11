// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.labels.combine]: a combined label nested many
// levels deep.  Each label-combining operator wraps the left operand's
// group_names in another level, and the union's walk must not grow
// exponentially with the depth: a nested union is asked for its own next
// name rather than iterated, so a step is one pass over the leaves.
//
// Two readers are checked: the library's iteration of group_names (the
// static_asserts), and Sema's reading of the label's groups.  Each label
// names "safety", which is ignored, so a precondition whose label's groups
// are read in full is not evaluated during constant evaluation; one whose
// groups are missed is enforced and makes the call non-constant.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-combined-label-nesting.C

#include <array>
#include <contracts>
#include <initializer_list>
#include <span>
#include <string_view>

namespace lbl = std::contracts::labels;
using lbl::operator|;
using lbl::operator""group;

template <typename R> constexpr int count_names(const R &r) {
  int n = 0;
  for (std::string_view s : r) {
    (void)s;
    ++n;
  }
  return n;
}

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

constexpr auto eight = "a"group | "b"group | "c"group | "d"group | "e"group |
                       "f"group | "g"group | "safety"group;

static_assert(count_names(eight.group_names) == 8);
static_assert(names_are(eight.group_names,
                        {"a", "b", "c", "d", "e", "f", "g", "safety"}));

constexpr int f(int x) pre<eight>(x > 0) { return x; }
static_assert(f(-1) == -1);

// Sixteen levels, with duplicates, in reverse order, and constituents of
// other range types.
struct sv_array_t {
  using assertion_control_object = sv_array_t;
  static constexpr std::array<std::string_view, 2> group_names = {"p", "b"};
};

constexpr const char *span_names[3] = {"q", "a", "q"};
struct span_t {
  using assertion_control_object = span_t;
  static constexpr std::span<const char *const> group_names{span_names};
};

constexpr auto sixteen = "o"group | "n"group | "m"group | "l"group | "k"group |
                         "j"group | "i"group | "h"group | sv_array_t{} |
                         "g"group | "f"group | "e"group | "d"group | span_t{} |
                         "c"group | "safety"group | "a"group;

static_assert(count_names(sixteen.group_names) == 18);
static_assert(names_are(sixteen.group_names,
                        {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
                         "k", "l", "m", "n", "o", "p", "q", "safety"}));

constexpr int g(int x) pre<sixteen>(x > 0) { return x; }
static_assert(g(-1) == -1);

// Nesting on both sides of an operator.
constexpr auto both = ("d"group | "c"group | "b"group | "a"group) |
                      ("safety"group | "x"group | "w"group | "v"group);
static_assert(names_are(both.group_names,
                        {"a", "b", "c", "d", "safety", "v", "w", "x"}));

constexpr int h(int x) pre<both>(x > 0) { return x; }
static_assert(h(-1) == -1);

// Control: no constituent names "safety", so the precondition is enforced.
constexpr auto unsafe = "a"group | "b"group | "c"group | "d"group | "e"group |
                        "f"group | "g"group | "h"group;
constexpr int u(int x) pre<unsafe>(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
static_assert(u(-1) == -1); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
