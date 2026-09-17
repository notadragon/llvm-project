// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// local_violation_label: labels with handle_contract_violation are detected.

#include <contracts>
using namespace std::contracts;
using namespace std::contracts::labels;

struct logging_t {
  using assertion_control_object = logging_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
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
  void handle_contract_violation(const contract_violation&) const {}
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
    return violation_handled::handled;
  }
};
constexpr static_handler_t static_handler{};
static_assert(local_violation_label<static_handler_t>);
void m(int x) pre<static_handler>(x > 0) {}

// Static void handler
struct static_void_handler_t {
  using assertion_control_object = static_void_handler_t;
  static void handle_contract_violation(const contract_violation&) {}
};
constexpr static_void_handler_t static_void_handler{};
static_assert(local_violation_label<static_void_handler_t>);
void n(int x) pre<static_void_handler>(x > 0) {}

// Combined: static | empty
void p(int x) pre<static_handler | empty_label>(x > 0) {}

// Combined: non-static | static
void q(int x) pre<logging | static_handler>(x > 0) {}
