// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o - | \
// RUN:   FileCheck %s

// P3400: the bypass is conservative.  None of these handlers is
// provably a bare rethrow of the in-flight exception, so every one of these
// checks keeps its EH region and still calls the _ex entry point.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-4.C)

#include <contracts>
#include <exception>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

bool boom();
void log_it();
extern int opaque;

// 1. Does something observable before rethrowing.
struct logs_first_t {
  using assertion_control_object = logs_first_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception) {
      log_it();
      throw;
    }
  }
};
constexpr logs_first_t logs_first{};
int f1(int i) pre<logs_first>(boom()) { return i; }

// 2. Returns instead of rethrowing: the exception would be swallowed, which
//    skipping the region would not do.
struct swallows_t {
  using assertion_control_object = swallows_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      return violation_handled::handled;
    return violation_handled::not_handled;
  }
};
constexpr swallows_t swallows{};
int f2(int i) pre<swallows>(boom()) { return i; }

// 3. Raises a different exception -- not the same thing as never catching.
struct translates_t {
  using assertion_control_object = translates_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw 42;
  }
};
constexpr translates_t translates{};
int f3(int i) pre<translates>(boom()) { return i; }

// 4. The branch is not decidable at the point of the check.
struct runtime_choice_t {
  using assertion_control_object = runtime_choice_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception && opaque)
      throw;
  }
};
constexpr runtime_choice_t runtime_choice{};
int f4(int i) pre<runtime_choice>(boom()) { return i; }

// 5. A noexcept handler cannot rethrow -- it would terminate.
struct noexcept_handler_t {
  using assertion_control_object = noexcept_handler_t;
  void handle_contract_violation(const contract_violation& v) const noexcept {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      log_it();
  }
};
constexpr noexcept_handler_t noexcept_handler{};
int f5(int i) pre<noexcept_handler>(boom()) { return i; }

// 6. A contract with no label at all has no local handler to reason about.
int f6(int i) pre(boom()) { return i; }

// 7. Delegation does not launder a side effect: the helper is followed, and
//    fails inside the nested walk exactly as it would at the top level.
inline void log_then_rethrow() {
  log_it();
  throw;
}

struct delegates_to_logger_t {
  using assertion_control_object = delegates_to_logger_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      log_then_rethrow();
  }
};
constexpr delegates_to_logger_t delegates_to_logger{};
int f7(int i) pre<delegates_to_logger>(boom()) { return i; }

// 8. A helper with no body available here cannot be followed.
void opaque_rethrow();

struct delegates_out_of_line_t {
  using assertion_control_object = delegates_out_of_line_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      opaque_rethrow();
  }
};
constexpr delegates_out_of_line_t delegates_out_of_line{};
int f8(int i) pre<delegates_out_of_line>(boom()) { return i; }

// 9. Mutual recursion terminates on the depth limit rather than hanging, and
//    yields no proof.
struct recursive_t {
  using assertion_control_object = recursive_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      handle_contract_violation(v);
  }
};
constexpr recursive_t recursive{};
int f9(int i) pre<recursive>(boom()) { return i; }

// 10. A combined label whose earlier component does something observable: the
//     rethrowing component is real, but it is not reached for free.
struct logs_and_declines_t {
  using assertion_control_object = logs_and_declines_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    log_it();
    return violation_handled::not_handled;
  }
};
struct rethrows_t {
  using assertion_control_object = rethrows_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
    return violation_handled::not_handled;
  }
};
constexpr logs_and_declines_t logs_and_declines{};
constexpr rethrows_t rethrows{};
int f10(int i) pre<logs_and_declines | rethrows>(boom()) { return i; }

// All ten keep both halves of the check.  The trailing CHECK-NOT pins the
// count exactly: one declaration plus ten call sites, and no eleventh.
// CHECK-COUNT-11: @__cxa_contract_violation_pre_enforce_ex
// CHECK-NOT: @__cxa_contract_violation_pre_enforce_ex
