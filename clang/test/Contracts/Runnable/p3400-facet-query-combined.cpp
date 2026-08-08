// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstddef>

using namespace std::contracts;
using namespace std::contracts::labels;

static int tag_key = 1;

struct tag_label_t {
  using assertion_control_object = tag_label_t;
  const char* tag;
  constexpr tag_label_t(const char* t) : tag(t) {}

  void* query(const void* key, std::size_t index) const {
    if (key == &tag_key && index == 0) return (void*)tag;
    return nullptr;
  }
};

constexpr tag_label_t alice("Alice");
constexpr tag_label_t bob("Bob");
constexpr tag_label_t carol("Carol");

// Left-associative: (alice | bob) | carol
void test_left(int x) pre<(alice | bob) | carol>(x > 0) { }

// Right-associative: alice | (bob | carol)
void test_right(int x) pre<alice | (bob | carol)>(x > 0) { }

static int test_num = 0;

void handle_contract_violation(const contract_violation& v) {
  ++test_num;
  std::printf("test %d:\n", test_num);
  for (std::size_t i = 0; i < 4; ++i) {
    auto* p = v.query_control_object(&tag_key, i);
    if (p)
      std::printf("  [%zu]=%s\n", i, (const char*)p);
    else
      std::printf("  [%zu]=null\n", i);
  }
}

int main() {
  // CHECK: test 1:
  // CHECK:   [0]=Alice
  // CHECK:   [1]=Bob
  // CHECK:   [2]=Carol
  // CHECK:   [3]=null
  test_left(-1);

  // CHECK: test 2:
  // CHECK:   [0]=Alice
  // CHECK:   [1]=Bob
  // CHECK:   [2]=Carol
  // CHECK:   [3]=null
  test_right(-1);
  return 0;
}
