// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags \
// RUN:   -S -emit-llvm -o - | FileCheck %s --check-prefix=ELIDED \
// RUN:     --implicit-check-not=__cxa_contract_violation_pre_enforce_ex
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-disable-rethrow-shortcut \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags \
// RUN:   -S -emit-llvm -o - | FileCheck %s --check-prefix=KEPT

// P3400: -fcontract-disable-rethrow-shortcut turns the optimization off, so a
// handler that would otherwise qualify gets its EH region back.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-rethrow-shortcut-7.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

bool boom();

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr rethrowing_t rethrowing{};

int f(int i) pre<rethrowing>(boom()) { return i; }

// ELIDED: @__cxa_contract_violation_pre_enforce_pf
// KEPT: @__cxa_contract_violation_pre_enforce_ex
