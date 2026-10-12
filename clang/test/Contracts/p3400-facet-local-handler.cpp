// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -fcontract-evaluation-semantic=observe -o %t && %t | FileCheck %s

// local_violation_label: labels with handle_contract_violation are detected,
// and run.  Each label handler prints its name and the global handler prints
// "global", so the output shows which ran: a label handler returning
// not_handled, or void, passes the violation on to the global handler; one
// returning handled does not.  A combined label runs its handlers right to
// left (p3400-local-handler-chain-order.cpp).

#include <contracts>
#include <cstdio>
using namespace std::contracts;
using namespace std::contracts::labels;

void handle_contract_violation(const contract_violation &) {
  std::printf(" global");
}

struct logging_t {
  using assertion_control_object = logging_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    std::printf(" logging");
    return violation_handled::not_handled;
  }
};
constexpr logging_t logging{};

static_assert(local_violation_label<logging_t>);
static_assert(!local_violation_label<empty_label_t>);

// Labels with handler compile correctly in contracts
void f(int x) pre<logging>(x > 0) {}

// Void-returning handler
struct void_handler_t {
  using assertion_control_object = void_handler_t;
  void handle_contract_violation(const contract_violation&) const {
    std::printf(" void_handler");
  }
};
constexpr void_handler_t void_handler{};
void g(int x) pre<void_handler>(x > 0) {}

// Combined: handler | empty
void h(int x) pre<logging | empty_label>(x > 0) {}

// Combined: empty | handler
void k(int x) pre<empty_label | logging>(x > 0) {}

// Static handler returning violation_handled
struct static_handler_t {
  using assertion_control_object = static_handler_t;
  static violation_handled handle_contract_violation(const contract_violation&) {
    std::printf(" static_handler");
    return violation_handled::handled;
  }
};
constexpr static_handler_t static_handler{};
static_assert(local_violation_label<static_handler_t>);
void m(int x) pre<static_handler>(x > 0) {}

// Static void handler
struct static_void_handler_t {
  using assertion_control_object = static_void_handler_t;
  static void handle_contract_violation(const contract_violation&) {
    std::printf(" static_void_handler");
  }
};
constexpr static_void_handler_t static_void_handler{};
static_assert(local_violation_label<static_void_handler_t>);
void n(int x) pre<static_void_handler>(x > 0) {}

// Combined: static | empty
void p(int x) pre<static_handler | empty_label>(x > 0) {}

// Combined: non-static | static
void q(int x) pre<logging | static_handler>(x > 0) {}

int main() {
  std::printf("f:");
  f(-1);
  std::printf("\n");
  std::printf("g:");
  g(-1);
  std::printf("\n");
  std::printf("h:");
  h(-1);
  std::printf("\n");
  std::printf("k:");
  k(-1);
  std::printf("\n");
  std::printf("m:");
  m(-1);
  std::printf("\n");
  std::printf("n:");
  n(-1);
  std::printf("\n");
  std::printf("p:");
  p(-1);
  std::printf("\n");
  std::printf("q:");
  q(-1);
  std::printf("\n");
}

// CHECK: f: logging global
// CHECK-NEXT: g: void_handler global
// CHECK-NEXT: h: logging global
// CHECK-NEXT: k: logging global
// CHECK-NEXT: m: static_handler
// CHECK-NEXT: n: static_void_handler global
// CHECK-NEXT: p: static_handler
// CHECK-NEXT: q: static_handler

// REQUIRES: contracts-libcxx, native
