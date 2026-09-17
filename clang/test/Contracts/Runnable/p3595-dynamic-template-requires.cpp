// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4283 -fcontract-configuration-file=%S/p3595-dynamic-template-requires.json %libcxx_flags -o %t && %t

// P3595 dynamic selection through a template instantiation whose contract ALSO
// carries a requires-clause (P4283).  requires-clauses are legal on templated
// contracts; a satisfied one keeps the contract, and the instantiated contract
// must still get its dynamic descriptor populated.
//
// Regression guard: the P4283 rejection is diagnosed only on the primary-parse
// path (Sema::ActOnContractAssert).  BuildContractStmt must NOT skip population
// for an instantiated contract just because it has a (satisfied, non-dependent)
// requires-clause -- otherwise isDynamic() stays false and codegen falls back to
// the static default ("ignore"), so no check runs and the selector is never
// consulted (violations == 0, which would abort here).
//
// Compile-time default is "ignore"; the runtime selector returns "observe", so
// the failing call must be counted exactly once (violations == 1) and execution
// continues.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_tpl_req_sel() { return evaluation_semantic::observe; }

template <class T>
concept Sized = sizeof(T) > 0;

template <class T>
void f(const T x) pre requires(Sized<T>) (x > 0) {}

int main() {
  f<int>(-1);
  if (violations != 1)
    std::abort();
  f<int>(1);
  if (violations != 1)
    std::abort();
}
