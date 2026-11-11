// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-template.json %libcxx_flags -o %t && %t

// P3595 dynamic selection through a *template instantiation*.  The contract
// lives in a function template, so it reaches codegen via
// TreeTransform::RebuildContractStmt -> Sema::BuildContractStmt rather than
// Sema::ActOnContractAssert.  The dynamic descriptor + transform table must be
// populated on that shared path too; otherwise the instantiated contract's
// isDynamic() stays false and codegen falls back to the static (eager)
// semantic -- here "ignore" -- so no check runs and the selector is never
// consulted.
//
// Compile-time default is "ignore"; the runtime selector returns "observe",
// so the failing call must be counted exactly once and execution continues.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_tpl_sel() { return evaluation_semantic::observe; }

template <class T>
void f(const T x) pre(x > 0) {}

int main() {
  f<int>(-1);
  if (violations != 1)
    std::abort();
  f<int>(1);
  if (violations != 1)
    std::abort();
}
