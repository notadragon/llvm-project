// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3100 \
// RUN:   -fcontracts-allow-assume -fcontracts-group-evaluation-semantic=safety:assume \
// RUN:   %libcxx_flags -o %t
// RUN: %t

// P3100 x P3400: a group-evaluation-semantic config selecting "assume" (with
// -fcontracts-allow-assume) resolves a grouped contract to assume -- codegen
// like ignore, so the predicate is not evaluated and no violation is reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-group-assume.C)

#include <contracts>

static int side = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const std::contracts::contract_violation&) {
  __builtin_abort();   // assume must not report a violation
}

void f_safety(int x) pre<"safety"group>(chk(x)) { }

int main() {
  f_safety(-1);                // group -> assume -> no check, predicate skipped
  if (side != 0) __builtin_abort();
}
