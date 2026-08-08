// D4301: a normal contract_assert violation has no CXA_FIELD_REPORT field, so
// contract_violation::report() must return nullptr (the populator is never
// invoked for ordinary violations).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-report-null.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4301 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation& v) {
  // CHECK: report=null
  std::printf(v.report() == nullptr ? "report=null\n" : "report=nonnull\n");
}

int f(int x) { contract_assert(x > 0); return x; }

int main() { f(0); }
