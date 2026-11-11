// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -g
// RUN: %t

#include <contracts>
#ifdef NDEBUG
#undef NDEBUG
#endif

struct FailureFlag {
  FailureFlag() = default;
  FailureFlag(FailureFlag const&) = delete;

  void expect_violation(bool value = true) {
    if (!(!have_expectation && !have_violation)) __builtin_abort();
    have_expectation = true;
    is_violation_expected = value;
  }

  void expect_none() {
    if (!(!have_expectation && !have_violation)) __builtin_abort();
    have_expectation = true;
    is_violation_expected = false;
  }

  void observe_violation() {
    if (!(have_expectation && !have_violation)) __builtin_abort();
    have_violation = true;
  }

  void finish() {
    if (!(have_expectation && (is_violation_expected == have_violation))) __builtin_abort();
    have_expectation = false;
    have_violation = false;
  }

  bool have_expectation = false;
  bool is_violation_expected = false;
  bool have_violation = false;

  ~FailureFlag() {
    if (!(!have_expectation || (have_expectation && is_violation_expected == have_violation))) __builtin_abort();
  }
};

constinit FailureFlag flag;
void handle_contract_violation(const std::contracts::contract_violation& cv) {
  flag.observe_violation();
}

struct A {}; // trivially copyable
struct B {   // not trivially copyable
  B() {}
  B(const B &) {}
};
template <typename T> T f(T * const ptr) post(r : &r == ptr) { return T{}; }
int main() {
  flag.expect_violation();
  A a = f(&a); // The postcondition check may fail.
  flag.finish();
  flag.expect_none();
  B b = f(&b); // The postcondition check is guaranteed to succeed.
  flag.finish();
}
