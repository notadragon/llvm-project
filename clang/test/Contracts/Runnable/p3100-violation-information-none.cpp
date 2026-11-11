// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   -fcontract-violation-information=none %libcxx_flags -o %t
// RUN: %t

// -fcontract-violation-information=none applies to the data block of an
// implicit contract assertion whose check code generation inserts (the null
// dereference check): an empty file and function name, line and column 0,
// an empty comment, no message; the kind and semantic are unchanged.
// p3100-violation-information-full.cpp is the same under =full.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/p3100-violation-information-none.C.

#include <contracts>
#include <cstdlib>

using namespace std::contracts;

void handle_contract_violation(const contract_violation &v) {
  if (v.location().file_name()[0] != '\0' ||
      v.location().function_name()[0] != '\0' || v.location().line() != 0 ||
      v.location().column() != 0 || v.comment()[0] != '\0' ||
      v.message() != nullptr || v.kind() != assertion_kind::implicit ||
      v.semantic() != evaluation_semantic::noexcept_observe)
    __builtin_abort();
  // The load would fault after an observed violation.
  std::_Exit(0);
}

int main() {
  int *volatile p = nullptr;
  volatile int r = *p;
  (void)r;
  __builtin_abort();
}
