// REQUIRES: contracts-libcxx, native
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A predicate that can throw only through an initializer list's array filler
// is emitted with the evaluation_exception try/catch, so the exception
// reaches the violation handler instead of escaping the function (CLANG-664:
// Sema::canThrow, which decides it, missed the filler).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-array-filler.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.detection_mode() ==
      std::contracts::detection_mode::evaluation_exception)
    ++handled;
}

struct S { S() { throw 2; } };
struct A { S s[2]; };
bool f(int x) pre(((void)A{}, x > 0)) { return true; }

int main() {
  int escaped = 0;
  try { f(1); } catch (...) { escaped = 1; }
  if (handled != 1 || escaped != 0)
    __builtin_abort();
}
