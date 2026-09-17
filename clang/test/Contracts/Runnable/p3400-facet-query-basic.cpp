// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstddef>

using namespace std::contracts;
using namespace std::contracts::labels;

static int name_key = 1;
static int email_key = 2;
static int unknown_key = 99;

struct owner_label_t {
  using assertion_control_object = owner_label_t;
  const char* name;
  const char* email;

  constexpr owner_label_t(const char* n, const char* e)
    : name(n), email(e) {}

  void* query(const void* key, std::size_t index) const {
    if (index != 0) return nullptr;
    if (key == &name_key) return (void*)name;
    if (key == &email_key) return (void*)email;
    return nullptr;
  }
};

constexpr owner_label_t owner("Alice", "alice@example.com");

void checked(int x) pre<owner>(x > 0) { }

void handle_contract_violation(const contract_violation& v) {
  // Known key, index 0
  auto* name_ptr = v.query_control_object(&name_key, 0);
  std::printf("name=%s\n", name_ptr ? (const char*)name_ptr : "(null)");

  // Known key (email)
  auto* email_ptr = v.query_control_object(&email_key, 0);
  std::printf("email=%s\n", email_ptr ? (const char*)email_ptr : "(null)");

  // Unknown key returns nullptr
  auto* unk = v.query_control_object(&unknown_key, 0);
  std::printf("unknown=%s\n", unk ? "non-null" : "null");

  // Known key, index > 0 returns nullptr (only one result)
  auto* idx1 = v.query_control_object(&name_key, 1);
  std::printf("index1=%s\n", idx1 ? "non-null" : "null");
}

int main() {
  // CHECK: name=Alice
  // CHECK: email=alice@example.com
  // CHECK: unknown=null
  // CHECK: index1=null
  checked(-1);
  return 0;
}
