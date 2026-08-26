// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=enforce -S -emit-llvm -o - | \
// RUN:   FileCheck %s --check-prefix=ENFORCE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=observe -S -emit-llvm -o - | \
// RUN:   FileCheck %s --check-prefix=OBSERVE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=noexcept_observe -S -emit-llvm -o - | \
// RUN:   FileCheck %s --check-prefix=NXOBSERVE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=noexcept_enforce -S -emit-llvm -o - | \
// RUN:   FileCheck %s --check-prefix=NXENFORCE

// P3400: the exception-detection entry point must be handed the *extended*
// data block -- the one carrying the label's local-handler and query pointers
// -- and not the bare global block.  Passing the global block is what left a
// throwing predicate unable to reach either facet.
//
// This is the codegen half of
// Runnable/p3400-local-handler-exception-path.cpp, and covers all four
// semantics that reach the entry points, which the runtime test does not.
// The label's handler returns rather than rethrowing, so the EH region is
// present to be inspected.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-local-handler-exception-path-codegen.C)

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

bool boom(); // Not noexcept: the predicate might throw.

struct label_t {
  using assertion_control_object = label_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    return violation_handled::not_handled;
  }
  void* query(const void*, std::size_t) const { return nullptr; }
};
constexpr label_t label{};

int f(int i) pre<label>(boom()) { return i; }

// The argument is the stack-allocated extended block, not the @contract.loc
// global.  The terminating semantics drop "noundef" because their entry point
// is noreturn, so it is optional here.
//
// ENFORCE: invoke void @__cxa_contract_violation_pre_enforce_ex(ptr {{(noundef )?}}%contract.ext.data
// OBSERVE: invoke void @__cxa_contract_violation_pre_observe_ex(ptr {{(noundef )?}}%contract.ext.data
// NXOBSERVE: invoke void @__cxa_contract_violation_pre_noexcept_observe_ex_noexcept(ptr {{(noundef )?}}%contract.ext.data
// NXENFORCE: invoke void @__cxa_contract_violation_pre_noexcept_enforce_ex_noexcept(ptr {{(noundef )?}}%contract.ext.data
