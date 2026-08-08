// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

// local_violation_label with static handler methods.

#include <contracts>
#include <cstdio>

using namespace std::contracts;
using namespace std::contracts::labels;

static int local_calls = 0;
static int global_calls = 0;

// Static handler returning violation_handled.
struct static_handled_t {
  using assertion_control_object = static_handled_t;
  static violation_handled handle_contract_violation(const contract_violation&) {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr static_handled_t static_handled{};

// Static handler returning not_handled.
struct static_not_handled_t {
  using assertion_control_object = static_not_handled_t;
  static violation_handled handle_contract_violation(const contract_violation&) {
    ++local_calls;
    return violation_handled::not_handled;
  }
};
constexpr static_not_handled_t static_not_handled{};

// Static handler returning void (treated as not_handled).
struct static_void_t {
  using assertion_control_object = static_void_t;
  static void handle_contract_violation(const contract_violation&) {
    ++local_calls;
  }
};
constexpr static_void_t static_void{};

// Non-static handler for combined tests.
struct nonstatic_handled_t {
  using assertion_control_object = nonstatic_handled_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    ++local_calls;
    return violation_handled::handled;
  }
};
constexpr nonstatic_handled_t nonstatic_handled{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void test_static_handled(int x) pre<static_handled>(x > 0) { }
void test_static_not_handled(int x) pre<static_not_handled>(x > 0) { }
void test_static_void(int x) pre<static_void>(x > 0) { }
void test_static_combined(int x) pre<(static_handled | empty_label)>(x > 0) { }
void test_empty_static(int x) pre<(empty_label | static_handled)>(x > 0) { }
void test_nonstatic_static(int x)
  pre<(nonstatic_handled | static_not_handled)>(x > 0) { }

int main() {
  // CHECK: static_handled: local=1 global=0
  test_static_handled(-1);
  std::printf("static_handled: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  // CHECK: static_not_handled: local=2 global=1
  test_static_not_handled(-1);
  std::printf("static_not_handled: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 2 || global_calls != 1) __builtin_abort();

  // CHECK: static_void: local=3 global=2
  test_static_void(-1);
  std::printf("static_void: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 3 || global_calls != 2) __builtin_abort();

  local_calls = global_calls = 0;

  // CHECK: static_combined: local=1 global=0
  test_static_combined(-1);
  std::printf("static_combined: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  local_calls = global_calls = 0;

  // CHECK: empty_static: local=1 global=0
  test_empty_static(-1);
  std::printf("empty_static: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();

  local_calls = global_calls = 0;

  // CHECK: nonstatic_static: local=1 global=0
  test_nonstatic_static(-1);
  std::printf("nonstatic_static: local=%d global=%d\n", local_calls, global_calls);
  if (local_calls != 1 || global_calls != 0) __builtin_abort();
}
