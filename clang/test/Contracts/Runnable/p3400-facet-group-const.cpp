// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=ignore \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// This deliberately does NOT pass -Wno-contract-invalid-label-facet.  It used
// to, which suppressed nothing -- the near-miss warning probes the function
// facets only, never the data-member ones -- while reading as though the
// non-const group_names below were diagnosed and were being silenced here,
// the opposite of the truth.

// P3400: group_names must be const, and combining sorts and de-duplicates.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-group-const.C)
//
// D3400R5 requires `is_const_v<decltype(t.group_names)>' so that nothing
// implies a label's group membership could change at run time and have an
// effect: the names are read during translation and never again.  Both the
// `static constexpr' and the const-non-static spellings qualify; a plain
// non-const member is not a facet, and the front end must agree with the
// concept rather than reading the member anyway.
//
// A combined label's group_names is the sorted, unique union of its
// constituents'.  The extent is a type-level upper bound -- the number of
// *distinct* names depends on values the constructor cannot see at class
// scope -- so de-duplication leaves empty rows at the end, which the front end
// skips.  That is why "safety | safety" must behave exactly like "safety".

#include <contracts>

namespace lbl = std::contracts::labels;
using lbl::operator|;

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

// The static spelling.
struct static_group_t {
  using assertion_control_object = static_group_t;
  static constexpr char group_names[][7] = {"safety"};
};
constexpr static_group_t static_group{};

// The const-non-static spelling: the name is fixed at construction.
struct member_group_t {
  using assertion_control_object = member_group_t;
  const char group_names[1][7];
  constexpr member_group_t(const char (&n)[7])
      : group_names{{n[0], n[1], n[2], n[3], n[4], n[5], n[6]}} {}
};
constexpr member_group_t member_group{"safety"};

// Non-const: not a facet at all.
struct mutable_group_t {
  using assertion_control_object = mutable_group_t;
  char group_names[1][7] = {"safety"};
};
constexpr mutable_group_t mutable_group{};

// A group the command line does not enable.
struct other_group_t {
  using assertion_control_object = other_group_t;
  static constexpr char group_names[][5] = {"perf"};
};
constexpr other_group_t other_group{};

static_assert(lbl::identification_label<static_group_t>);
static_assert(lbl::identification_label<member_group_t>);
static_assert(!lbl::identification_label<mutable_group_t>);

// Combining preserves participation, in either order and with itself.
constexpr auto safety_perf = static_group | other_group;
constexpr auto perf_safety = other_group | static_group;
constexpr auto safety_twice = static_group | static_group;
static_assert(lbl::identification_label<decltype(safety_perf)>);
static_assert(lbl::identification_label<decltype(safety_twice)>);

void f_static(int x) pre<static_group>(x > 0) {}
void f_member(int x) pre<member_group>(x > 0) {}
void f_mutable(int x) pre<mutable_group>(x > 0) {}
void f_other(int x) pre<other_group>(x > 0) {}
void f_sp(int x) pre<safety_perf>(x > 0) {}
void f_ps(int x) pre<perf_safety>(x > 0) {}
void f_twice(int x) pre<safety_twice>(x > 0) {}

// The union is sorted, so "perf" precedes "safety" regardless of the order the
// operands were written in.
static_assert(safety_perf.group_names[0][0] == 'p');
static_assert(safety_perf.group_names[1][0] == 's');
static_assert(perf_safety.group_names[0][0] == 'p');
static_assert(perf_safety.group_names[1][0] == 's');

// Duplicates collapse: one name, and the spare row is empty.
static_assert(safety_twice.group_names[0][0] == 's');
static_assert(safety_twice.group_names[1][0] == '\0');

int main() {
  // Only the "safety" group is enabled, so these fire.
  reported = 0;
  f_static(-1);
  if (reported != 1)
    __builtin_abort();
  reported = 0;
  f_member(-1);
  if (reported != 1)
    __builtin_abort();

  // A non-const group_names is not a facet, so the label names no groups and
  // the assertion stays at the default `ignore'.
  reported = 0;
  f_mutable(-1);
  if (reported != 0)
    __builtin_abort();

  // A group that was not enabled stays ignored.
  reported = 0;
  f_other(-1);
  if (reported != 0)
    __builtin_abort();

  // Combined labels are in every constituent's groups, either order.
  reported = 0;
  f_sp(-1);
  if (reported != 1)
    __builtin_abort();
  reported = 0;
  f_ps(-1);
  if (reported != 1)
    __builtin_abort();

  // De-duplication does not lose the group, and the empty spare row must not
  // register as a group of its own.
  reported = 0;
  f_twice(-1);
  if (reported != 1)
    __builtin_abort();

  return 0;
}
