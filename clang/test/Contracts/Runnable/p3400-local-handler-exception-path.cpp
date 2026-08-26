// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=observe -o %t
// RUN: %t

// P3400: a violation detected because the predicate exited via an exception
// must reach the label's facets exactly as a false predicate does -- the local
// violation handler is dispatched to, it can claim the violation and so
// suppress the global handler, and query_control_object still answers.
//
// Regression: CodeGen passed the *basic* global data block to the _ex entry
// point while building the extended block -- the one carrying the local
// handler and query pointers -- only on the predicate-false path.  So for a
// throwing predicate the local handler was never called and
// query_control_object silently answered null, for every assertion kind.  GCC
// has always passed one shared block to both entry points.
//
// Every handler here returns rather than rethrowing, so the EH region is
// always present to be exercised regardless of any later shortcut over it.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-local-handler-exception-path.C)

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int query_key = 1;
static const char query_answer[] = "answered";

static int local_calls = 0;
static int global_calls = 0;
static int last_local_mode = 0;
static int last_global_mode = 0;
static const char* last_global_query = nullptr;

// Declines, so the global handler still runs and can probe the query facet.
struct declining_t {
  using assertion_control_object = declining_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    ++local_calls;
    last_local_mode = (int)v.detection_mode();
    return violation_handled::not_handled;
  }
  void* query(const void* key, std::size_t index) const {
    if (key == &query_key && index == 0)
      return (void*)query_answer;
    return nullptr;
  }
};

// Claims the violation, which must stop the global handler.
struct claiming_t {
  using assertion_control_object = claiming_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    ++local_calls;
    last_local_mode = (int)v.detection_mode();
    return violation_handled::handled;
  }
};

// No handler of its own; only there to be combined.
struct comment_t {
  using assertion_control_object = comment_t;
  constexpr const char* compute_comment(const char* c) const { return c; }
};

constexpr declining_t declining{};
constexpr claiming_t claiming{};
constexpr comment_t comment{};

void handle_contract_violation(const contract_violation& v) {
  ++global_calls;
  last_global_mode = (int)v.detection_mode();
  last_global_query = (const char*)v.query_control_object(&query_key, 0);
}

struct E {};
static bool boom() { throw E{}; }

// Every assertion kind, with a predicate that throws.
int pre_throws(int i) pre<declining>(boom()) { return i; }
int post_throws(const int i) post<declining>(r : boom()) { return i; }
int assert_throws(int i) {
  contract_assert<declining>(boom());
  return i;
}

// The same label with a merely false predicate, as the control.
int pre_false(int i) pre<declining>(i > 0) { return i; }

// Claiming on the exception path must suppress the global handler.
int pre_claims(int i) pre<claiming>(boom()) { return i; }

// Reached through __combined_label rather than directly.
int pre_combined(int i) pre<declining | comment>(boom()) { return i; }

static void reset() {
  local_calls = global_calls = 0;
  last_local_mode = last_global_mode = 0;
  last_global_query = nullptr;
}

static void run_throwing(int (*fn)(int)) {
  reset();
  try {
    fn(1);
  } catch (const E&) {
  }
}

int main() {
  // -- the local handler is dispatched to on the exception path, for each
  //    assertion kind, and sees the right detection mode.
  for (auto* fn : {pre_throws, post_throws, assert_throws}) {
    run_throwing(fn);
    if (local_calls != 1)
      __builtin_abort();
    if (last_local_mode != (int)detection_mode::evaluation_exception)
      __builtin_abort();
    // It declined, so the global handler ran too, with the same mode ...
    if (global_calls != 1)
      __builtin_abort();
    if (last_global_mode != (int)detection_mode::evaluation_exception)
      __builtin_abort();
    // ... and could still reach the query facet.
    if (!last_global_query || __builtin_strcmp(last_global_query, query_answer))
      __builtin_abort();
  }

  // -- the predicate-false path is unchanged.
  reset();
  pre_false(-1);
  if (local_calls != 1 || global_calls != 1)
    __builtin_abort();
  if (last_local_mode != (int)detection_mode::predicate_false)
    __builtin_abort();
  if (!last_global_query || __builtin_strcmp(last_global_query, query_answer))
    __builtin_abort();

  // -- claiming the violation on the exception path suppresses the global
  //    handler, exactly as it does for a false predicate.
  run_throwing(pre_claims);
  if (local_calls != 1 || global_calls != 0)
    __builtin_abort();
  if (last_local_mode != (int)detection_mode::evaluation_exception)
    __builtin_abort();

  // -- and it works through a combined label.
  run_throwing(pre_combined);
  if (local_calls != 1 || global_calls != 1)
    __builtin_abort();
  if (last_local_mode != (int)detection_mode::evaluation_exception)
    __builtin_abort();
}
