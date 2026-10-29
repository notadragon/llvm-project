// REQUIRES: contracts-libcxx, native
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A predicate that can throw only through the destructor a delete-expression
// invokes is emitted with the evaluation_exception try/catch, so the
// exception reaches the violation handler instead of escaping the function
// (CLANG-665: CodeGen's own walk missed the destructor; the answer is now
// Sema::canThrow's, kept on the contract).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-delete.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.detection_mode() ==
      std::contracts::detection_mode::evaluation_exception)
    ++handled;
}

struct D { ~D() noexcept(false) { throw 1; } };
bool f(D *p) pre((delete p, true)) { return true; }

int main() {
  int escaped = 0;
  try { f(new D); } catch (...) { escaped = 1; }
  if (handled != 1 || escaped != 0)
    __builtin_abort();
}
