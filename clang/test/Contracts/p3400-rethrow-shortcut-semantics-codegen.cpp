// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags \
// RUN:   -S -emit-llvm -o - | FileCheck %s

// P3400: the rethrow shortcut is restricted to enforce and observe.  The
// noexcept semantics (D4298) exist to guarantee nothing propagates out of a
// check, so their EH region must survive even for a rethrowing handler.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-rethrow-shortcut-5.C)

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

// CHECK: @__cxa_contract_violation_pre_noexcept_observe_ex_noexcept
