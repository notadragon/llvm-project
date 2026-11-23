// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -S -emit-llvm -o - | \
// RUN:   FileCheck %s

// P3400: the rethrow-bypass analysis follows a switch on detection_mode()
// (notadragon_wg21 DECISIONS-resolved.md E62 (b)).  The condition folds, the
// matching case or the default is found, and the walk falls through the
// labels that follow until a break leaves the switch.  Each handler in the
// first group provably rethrows an evaluation_exception, so its check has no
// EH region and no _ex entry point; each in the second group does not, or
// cannot be proved to, and keeps both.  Every check keeps its
// predicate-false path.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-switch-codegen.C
// in the gnu_gcc fork.

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;

bool boom(); // Not noexcept: the predicate might throw.
void log_it();
extern int opaque;

// Rethrows.

// 1. The natural shape.
struct plain_t {
  using assertion_control_object = plain_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::evaluation_exception:
      throw;
    default:
      break;
    }
  }
};
constexpr plain_t plain{};
int p1(int i) pre<plain>(boom()) { return i; }

// 2. Through the default, with fall-through from an empty case.
struct via_default_t {
  using assertion_control_object = via_default_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::predicate_false:
      log_it();
      break;
    case detection_mode::unspecified:
    default:
      throw;
    }
  }
};
constexpr via_default_t via_default{};
int p2(int i) pre<via_default>(boom()) { return i; }

// 3. A break inside an untaken if, then a rethrow after the switch.
struct after_t {
  using assertion_control_object = after_t;
  violation_handled
  handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::predicate_false:
      return violation_handled::handled;
    case detection_mode::evaluation_exception:
      if (v.detection_mode() != detection_mode::evaluation_exception)
        return violation_handled::handled;
      break;
    default:
      break;
    }
    throw;
  }
};
constexpr after_t after{};
int p3(int i) pre<after>(boom()) { return i; }

// 4. A nested switch, whose break leaves only the inner one.
struct nested_t {
  using assertion_control_object = nested_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::evaluation_exception:
      switch (v.semantic()) {
      default:
        break;
      }
      throw;
    default:
      break;
    }
  }
};
constexpr nested_t nested{};
int p4(int i) pre<nested>(boom()) { return i; }

// Keeps the EH region.

// 5. The matching case breaks without rethrowing.
struct breaks_t {
  using assertion_control_object = breaks_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::evaluation_exception:
      break;
    default:
      throw;
    }
  }
};
constexpr breaks_t breaks{};
int k5(int i) pre<breaks>(boom()) { return i; }

// 6. No case matches and there is no default: the switch does nothing.
struct no_match_t {
  using assertion_control_object = no_match_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::predicate_false:
      throw;
    }
  }
};
constexpr no_match_t no_match{};
int k6(int i) pre<no_match>(boom()) { return i; }

// 7. The condition does not fold.
struct opaque_t {
  using assertion_control_object = opaque_t;
  void handle_contract_violation(const contract_violation &) const {
    switch (opaque) {
    case 1:
      throw;
    default:
      break;
    }
  }
};
constexpr opaque_t opaque_label{};
int k7(int i) pre<opaque_label>(boom()) { return i; }

// 8. Something observable on the way through the matching case.
struct logs_t {
  using assertion_control_object = logs_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    case detection_mode::evaluation_exception:
      log_it();
      throw;
    default:
      break;
    }
  }
};
constexpr logs_t logs{};
int k8(int i) pre<logs>(boom()) { return i; }

// 9. A case label nested inside a statement (irregular): not followed.
struct irregular_t {
  using assertion_control_object = irregular_t;
  void handle_contract_violation(const contract_violation &v) const {
    switch (v.detection_mode()) {
    default:
      if (opaque) {
      case detection_mode::evaluation_exception:
        throw;
      }
    }
  }
};
constexpr irregular_t irregular{};
int k9(int i) pre<irregular>(boom()) { return i; }

// CHECK-LABEL: define {{.*}} @_Z2p1i(
// CHECK: @__cxa_contract_violation_pre_enforce_pf
// CHECK-NOT: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2p2i(
// CHECK: @__cxa_contract_violation_pre_enforce_pf
// CHECK-NOT: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2p3i(
// CHECK: @__cxa_contract_violation_pre_enforce_pf
// CHECK-NOT: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2p4i(
// CHECK: @__cxa_contract_violation_pre_enforce_pf
// CHECK-NOT: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2k5i(
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_pf
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2k6i(
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_pf
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2k7i(
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_pf
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2k8i(
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_pf
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_ex
// CHECK-LABEL: define {{.*}} @_Z2k9i(
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_pf
// CHECK-DAG: @__cxa_contract_violation_pre_enforce_ex

// REQUIRES: contracts-libcxx
