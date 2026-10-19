// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstddef>

using namespace std::contracts;
using namespace std::contracts::labels;

static int info_key = 1;
static int handler_calls = 0;
static int global_calls = 0;

struct full_label_t {
  using assertion_control_object = full_label_t;
  const char* info;

  constexpr full_label_t(const char* i) : info(i) {}

  violation_handled handle_contract_violation(const contract_violation& v) const {
    ++handler_calls;
    std::printf("local handler: %s\n", info);
    return violation_handled::handled;
  }

  void* query(const void* key, std::size_t index) const {
    if (key == &info_key && index == 0) return (void*)info;
    return nullptr;
  }
};

constexpr full_label_t full_label("full-data");

void checked(int x) pre<full_label>(x > 0) { }

void handle_contract_violation(const contract_violation& v) {
  ++global_calls;
  auto* p = v.query_control_object(&info_key, 0);
  std::printf("global handler: query=%s\n", p ? (const char*)p : "(null)");
}

int main() {
  // The local handler should be called (and return handled),
  // so the global handler should NOT be called.
  // But the query function should still be accessible.
  // CHECK: local handler: full-data
  checked(-1);

  // CHECK: handler_calls=1
  // CHECK: global_calls=0
  std::printf("handler_calls=%d\n", handler_calls);
  std::printf("global_calls=%d\n", global_calls);
  if (handler_calls != 1 || global_calls != 0) __builtin_abort();
  return 0;
}
