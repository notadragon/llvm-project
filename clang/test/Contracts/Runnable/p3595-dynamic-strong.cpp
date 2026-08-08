// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-strong.json %libcxx_flags -o %t && %t

// P3595 dynamic selection: a strong user-provided selector determines the
// runtime semantic.  The config compile-time default is "ignore", but the
// selector returns "observe", so the violation is handled and execution
// continues.  The selector is called at run time (not the compile-time
// default), so violations must be counted exactly once for the failing call.

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int violations = 0;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_sel() { return evaluation_semantic::observe; }

void f(const int x) pre(x > 0) {}

int main() {
  f(-1);
  if (violations != 1)
    std::abort();
  f(1);
  if (violations != 1)
    std::abort();
}
