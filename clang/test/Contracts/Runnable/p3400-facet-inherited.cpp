// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: a facet member inherited from a base class is a facet.
//
// The concepts use ordinary member lookup, which sees inherited members, and
// Sema agreed -- but CodeGen scanned direct members only, so an inherited
// handler or query was detected and then no trampoline was ever built.  The
// facet silently did nothing: the label looked applied and the member was
// never called.  compute_comment, compute_message and compute_semantic were
// unaffected because they are consumed in Sema and never reach that scan.
//
// The offset cases are the point of this test as much as the inheritance is.
// Both trampolines pass the label pointer straight through as `this', which
// is only correct when the method belongs to the most-derived class or to a
// base at offset zero.  Under multiple inheritance the base sits at a
// non-zero offset, so the pointer has to be stepped to it -- and a handler
// reading its own member is the only way to notice if it was not, since an
// unadjusted `this' still calls the right function, just on the wrong bytes.

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
namespace lbl = std::contracts::labels;

static int key = 1;
static int global_calls = 0;
static int local_calls = 0;
static int observed_tag = 0;
static bool last_query = false;

void handle_contract_violation(const contract_violation &v) {
  ++global_calls;
  last_query = v.query_control_object(&key, 0) != nullptr;
}

// ---------------------------------------------------------------------------
// Single inheritance: the base is at offset zero.
// ---------------------------------------------------------------------------

struct HandlerBase {
  void handle_contract_violation(const contract_violation &) const {
    ++local_calls;
  }
};
struct inherited_handler_t : HandlerBase {
  using assertion_control_object = inherited_handler_t;
};

struct QueryBase {
  void *query(const void *k, std::size_t) const {
    return k == &key ? (void *)&key : nullptr;
  }
};
struct inherited_query_t : QueryBase {
  using assertion_control_object = inherited_query_t;
};

static_assert(lbl::local_violation_label<inherited_handler_t>);
static_assert(lbl::queryable_label<inherited_query_t>);

constexpr inherited_handler_t inherited_handler{};
constexpr inherited_query_t inherited_query{};

void f_handler(int x) pre<inherited_handler>(x > 0) {}
void f_query(int x) pre<inherited_query>(x > 0) {}

// ---------------------------------------------------------------------------
// Multiple inheritance: the facet base sits at a non-zero offset.  Each member
// reads its own state, so a `this' that was not stepped to the base reads the
// padding instead and the check fails.
// ---------------------------------------------------------------------------

struct Pad {
  char filler[16] = {};
};

struct TaggedHandlerBase {
  int tag = 0xABCD;
  void handle_contract_violation(const contract_violation &) const {
    ++local_calls;
    observed_tag = tag;
  }
};
struct offset_handler_t : Pad, TaggedHandlerBase {
  using assertion_control_object = offset_handler_t;
};

struct TaggedQueryBase {
  int tag = 0xABCD;
  void *query(const void *k, std::size_t) const {
    if (k != &key || tag != 0xABCD)
      return nullptr;
    return (void *)&key;
  }
};
struct offset_query_t : Pad, TaggedQueryBase {
  using assertion_control_object = offset_query_t;
};

static_assert(lbl::local_violation_label<offset_handler_t>);
static_assert(lbl::queryable_label<offset_query_t>);

constexpr offset_handler_t offset_handler{};
constexpr offset_query_t offset_query{};

void f_offset_handler(int x) pre<offset_handler>(x > 0) {}
void f_offset_query(int x) pre<offset_query>(x > 0) {}

// ---------------------------------------------------------------------------
// A direct member hides an inherited one of the same name.
// ---------------------------------------------------------------------------

struct hiding_t : HandlerBase {
  using assertion_control_object = hiding_t;
  void handle_contract_violation(const contract_violation &) const {
    local_calls += 100;
  }
};
static_assert(lbl::local_violation_label<hiding_t>);
constexpr hiding_t hiding{};
void f_hiding(int x) pre<hiding>(x > 0) {}

// ---------------------------------------------------------------------------
// Inherited compute_* facets, which always worked; kept so the whole family is
// pinned in one place.
// ---------------------------------------------------------------------------

struct SemBase {
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const {
    return evaluation_semantic::ignore;
  }
};
struct inherited_sem_t : SemBase {
  using assertion_control_object = inherited_sem_t;
};
static_assert(lbl::semantic_computation_label<inherited_sem_t>);
constexpr inherited_sem_t inherited_sem{};
void f_sem(int x) pre<inherited_sem>(x > 0) {}

static void reset() {
  global_calls = local_calls = observed_tag = 0;
  last_query = false;
}

int main() {
  reset();
  f_handler(-1);
  if (local_calls != 1)
    __builtin_abort();

  reset();
  f_query(-1);
  if (!last_query)
    __builtin_abort();

  reset();
  f_offset_handler(-1);
  if (local_calls != 1 || observed_tag != 0xABCD)
    __builtin_abort();

  reset();
  f_offset_query(-1);
  if (!last_query)
    __builtin_abort();

  reset();
  f_hiding(-1);
  if (local_calls != 100)
    __builtin_abort();

  reset();
  f_sem(-1);
  if (global_calls != 0)
    __builtin_abort();

  return 0;
}
