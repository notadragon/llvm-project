// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=ignore \
// RUN:   -fcontract-group-evaluation-semantic=safety:observe \
// RUN:   %libcxx_flags -o %t && %t

// P3400 [support.contract.control.ident]: a label's group_names selects the
// semantic of a run-time check, read as a range-based for would walk it --
// an array, or begin and end as members or found by argument-dependent
// lookup -- with elements converted to const char * when no <contracts> or
// <string_view> is in scope.  A group_names that is not const, or whose
// elements do not convert, names no groups.
//
// Only the "safety" group is observed and everything else is ignored, so the
// handler runs exactly for the labels whose names, read in full, include
// "safety".
//
// Constant-evaluation counterpart: ../p3400-identification-label-range-no-header.cpp.
//
// Mirror: tests/tests/contracts/p3400groups/ranges.sft.cpp (edgcpp_compiler)

namespace std {
namespace contracts {
class contract_violation;
}
} // namespace std

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

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
  const char *group_names[1] = {"safety"};
};

// Elements that do not convert: no groups.
struct int_array_t {
  using assertion_control_object = int_array_t;
  static constexpr int group_names[2] = {1, 2};
};

void a(int x) pre<member_t{}>(x > 0) {}
void b(int x) pre<adl_t{}>(x > 0) {}
void c(int x) pre<name_array_t{}>(x > 0) {}
void d(int x) pre<ptr_array_t{}>(x > 0) {}
void e(int x) pre<short_t{}>(x > 0) {}
void f(int x) pre<nonconst_t{}>(x > 0) {}
void g(int x) pre<int_array_t{}>(x > 0) {}

#define EXPECT(call, n)                                                        \
  do {                                                                         \
    reported = 0;                                                              \
    call;                                                                      \
    if (reported != (n))                                                       \
      __builtin_abort();                                                       \
  } while (0)

int main() {
  EXPECT(a(0), 1);
  EXPECT(b(0), 1);
  EXPECT(c(0), 1);
  EXPECT(d(0), 1);
  EXPECT(e(0), 0);
  EXPECT(f(0), 0);
  EXPECT(g(0), 0);
}
