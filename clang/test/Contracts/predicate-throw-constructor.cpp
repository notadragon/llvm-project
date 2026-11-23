// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A predicate that can throw only through a constructor is emitted with the
// evaluation_exception try/catch, so the exception reaches the violation
// handler instead of escaping the function.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-not-detected.C

#include <contracts>
#include <new>
#include <typeinfo>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.detection_mode() ==
      std::contracts::detection_mode::evaluation_exception)
    ++handled;
}

struct T { T(int) { throw 1; } bool ok() const noexcept { return true; } };
bool f(int x) pre(T(x).ok()) { return true; }

int main() {
  int escaped = 0;
  try { f(-1); } catch (...) { escaped = 1; }
  if (handled != 1 || escaped != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
