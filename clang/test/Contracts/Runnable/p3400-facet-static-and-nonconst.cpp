// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: a facet member must be const or static, and both must work.
//
// Facets are invoked on a constexpr -- therefore const -- assertion-control
// object, so a concept accepts the call when the member is const or static,
// and rejects a non-const one.  Two bugs sat on opposite sides of that:
//
//   - A static `query' crashed the compiler.  Detection called
//     BuildCallToMemberFunction, which asserts that its callee has
//     bound-member or overload type; the member reference for a static member
//     is an ordinary function lvalue, so the assertion fired.  Detection now
//     goes through BuildCallExpr, which dispatches on the callee's real form.
//
//   - A non-const `handle_contract_violation' was accepted as a facet, and the
//     concept says it is not one.  Detection only formed a member reference,
//     which succeeds for a member of any signature and, on a const object,
//     succeeds even for a non-const member: the constness of the implicit
//     object argument is not checked until a call is built.  Detection now
//     builds the call the concept describes.
//
// Combining a label made both louder, since __combined_label dispatches on the
// concepts: the bare label and the combined one disagreed about the same type.
// The last section pins that they agree.

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
namespace lbl = std::contracts::labels;
using std::contracts::labels::operator|;

static int key = 1;
static int global_calls = 0;
static int local_calls = 0;
static bool last_query = false;

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  last_query = v.query_control_object(&key, 0) != nullptr;
}

// ---------------------------------------------------------------------------
// static members are facets.
// ---------------------------------------------------------------------------

struct static_handler_t {
  using assertion_control_object = static_handler_t;
  static void handle_contract_violation(const contract_violation &) {
    ++local_calls;
  }
};

// This one used to crash the compiler outright.
struct static_query_t {
  using assertion_control_object = static_query_t;
  static void *query(const void *k, std::size_t) {
    return k == &key ? (void *)&key : nullptr;
  }
};

struct static_comment_t {
  using assertion_control_object = static_comment_t;
  static constexpr const char *compute_comment(const char *) { return "S"; }
};

struct static_sem_t {
  using assertion_control_object = static_sem_t;
  static constexpr evaluation_semantic compute_semantic(evaluation_semantic) {
    return evaluation_semantic::ignore;
  }
};

static_assert(lbl::local_violation_label<static_handler_t>);
static_assert(lbl::queryable_label<static_query_t>);
static_assert(lbl::compute_comment_label<static_comment_t>);
static_assert(lbl::semantic_computation_label<static_sem_t>);

constexpr static_handler_t static_handler{};
constexpr static_query_t static_query{};
constexpr static_sem_t static_sem{};

void f_static_handler(int x) pre<static_handler>(x > 0) {}
void f_static_query(int x) pre<static_query>(x > 0) {}
void f_static_sem(int x) pre<static_sem>(x > 0) {}

// ---------------------------------------------------------------------------
// non-const members are not facets.
// ---------------------------------------------------------------------------

struct mut_handler_t {
  using assertion_control_object = mut_handler_t;
  void handle_contract_violation(const contract_violation &) {
    local_calls += 100;
  }
};

struct mut_query_t {
  using assertion_control_object = mut_query_t;
  void *query(const void *k, std::size_t) { return (void *)k; }
};

static_assert(!lbl::local_violation_label<mut_handler_t>);
static_assert(!lbl::queryable_label<mut_query_t>);

constexpr mut_handler_t mut_handler{};
constexpr mut_query_t mut_query{};

void f_mut_handler(int x) pre<mut_handler>(x > 0) {}
void f_mut_query(int x) pre<mut_query>(x > 0) {}

// ---------------------------------------------------------------------------
// A combined label must reach the same conclusion as the bare one.
// ---------------------------------------------------------------------------

struct plain_t {
  using assertion_control_object = plain_t;
};
constexpr plain_t plain{};

constexpr auto combined_static = static_handler | plain;
constexpr auto combined_mut = mut_handler | plain;

static_assert(lbl::local_violation_label<decltype(combined_static)>);
static_assert(!lbl::local_violation_label<decltype(combined_mut)>);

void f_combined_static(int x) pre<combined_static>(x > 0) {}
void f_combined_mut(int x) pre<combined_mut>(x > 0) {}

static void reset() {
  global_calls = local_calls = 0;
  last_query = false;
}

int main() {
  reset();
  f_static_handler(-1);
  if (local_calls != 1)
    __builtin_abort();

  reset();
  f_static_query(-1);
  if (!last_query)
    __builtin_abort();

  reset();
  f_static_sem(-1);
  if (global_calls != 0)
    __builtin_abort();

  // A non-const member is not a facet, so the local handler never runs and the
  // global one reports as usual.
  reset();
  f_mut_handler(-1);
  if (local_calls != 0 || global_calls != 1)
    __builtin_abort();

  reset();
  f_mut_query(-1);
  if (last_query)
    __builtin_abort();

  // Combining does not change the answer in either direction.
  reset();
  f_combined_static(-1);
  if (local_calls != 1)
    __builtin_abort();

  reset();
  f_combined_mut(-1);
  if (local_calls != 0 || global_calls != 1)
    __builtin_abort();

  return 0;
}
