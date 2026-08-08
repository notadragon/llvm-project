// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstddef>

using namespace std::contracts;
using namespace std::contracts::labels;

static int some_key = 1;

// A non-queryable label (no query method).
struct plain_label_t {
  using assertion_control_object = plain_label_t;
};
constexpr plain_label_t plain{};

void with_plain_label(int x) pre<plain>(x > 0) { }
void without_label(int x) pre(x > 0) { }

static int test_num = 0;

void handle_contract_violation(const contract_violation& v) {
  ++test_num;
  auto* p = v.query_control_object(&some_key, 0);
  std::printf("test %d: query=%s\n", test_num, p ? "non-null" : "null");
}

int main() {
  // CHECK: test 1: query=null
  with_plain_label(-1);

  // CHECK: test 2: query=null
  without_label(-1);
  return 0;
}
