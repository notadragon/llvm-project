// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: combining labels that carry a local violation handler.  A handler may
// return void or violation_handled; a void one never claims the violation, so
// the next component still runs and the global handler still reports.  A
// handler returning "handled" claims it, which short-circuits both the
// remaining components and the global handler.
//
// Regression: __combined_label::handle_contract_violation declared
// `auto __r = _M_lhs.handle_contract_violation(__v);' and only then asked
// whether decltype(__r) was void -- too late, since deducing void for a
// variable is already ill-formed.  Combining any void-returning handler failed
// to compile with "variable has incomplete type 'void'".
// (GCC mirror: g++.dg/contracts/cpp26/p3400-combine-local-handler.C)

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int trace[8];
static int trace_len = 0;
static int global_calls = 0;
static const char* last_comment = "";

static void note(int id) { trace[trace_len++] = id; }

void handle_contract_violation(const contract_violation& v) {
  ++global_calls;
  last_comment = v.comment();
}

// Two void-returning handlers: neither can ever claim the violation.
struct void_a_t {
  using assertion_control_object = void_a_t;
  void handle_contract_violation(const contract_violation&) const { note(1); }
};
struct void_b_t {
  using assertion_control_object = void_b_t;
  void handle_contract_violation(const contract_violation&) const { note(2); }
};

// Two returning handlers, one declining and one claiming.
struct declines_t {
  using assertion_control_object = declines_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    note(3);
    return violation_handled::not_handled;
  }
};
struct claims_t {
  using assertion_control_object = claims_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    note(4);
    return violation_handled::handled;
  }
};

// A label with no handler at all, to combine against.
struct comment_t {
  using assertion_control_object = comment_t;
  constexpr const char* compute_comment(const char*) const { return "tagged"; }
};

constexpr void_a_t void_a{};
constexpr void_b_t void_b{};
constexpr declines_t declines{};
constexpr claims_t claims{};
constexpr comment_t comment{};

void f_vv(int x) pre<void_a | void_b>(x > 0) {}
void f_vno(int x) pre<void_a | declines>(x > 0) {}
void f_vyes(int x) pre<void_a | claims>(x > 0) {}
void f_yesv(int x) pre<claims | void_a>(x > 0) {}
void f_nov(int x) pre<declines | void_a>(x > 0) {}
void f_vn(int x) pre<void_a | comment>(x > 0) {}
void f_nv(int x) pre<comment | void_a>(x > 0) {}
void f_three(int x) pre<void_a | comment | claims>(x > 0) {}

static void check(void (*fn)(int), const char* expected, int expect_global) {
  trace_len = 0;
  global_calls = 0;
  fn(-1);

  const int n = (int)__builtin_strlen(expected);
  if (trace_len != n)
    __builtin_abort();
  for (int i = 0; i < n; ++i)
    if (trace[i] != expected[i] - '0')
      __builtin_abort();
  if (global_calls != expect_global)
    __builtin_abort();
}

int main() {
  // Void handlers never claim: both components run, and the violation is
  // still reported globally.
  check(f_vv, "12", 1);
  check(f_vno, "13", 1);

  // A claiming handler stops the global report ...
  check(f_vyes, "14", 0);

  // ... and stops later components from running at all.
  check(f_yesv, "4", 0);

  // A declining handler does not.
  check(f_nov, "31", 1);

  // Combining with a label that has no handler leaves the handler in place
  // and still applies the other label's facet.
  check(f_vn, "1", 1);
  if (__builtin_strcmp(last_comment, "tagged") != 0)
    __builtin_abort();
  check(f_nv, "1", 1);
  if (__builtin_strcmp(last_comment, "tagged") != 0)
    __builtin_abort();

  // Three-way: the void handler runs, the comment label contributes nothing
  // here, and the claiming handler ends it.
  check(f_three, "14", 0);
}
