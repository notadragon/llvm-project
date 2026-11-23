// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags -o %t
// RUN: %t

// -fcontract-violation-information=full, the default: the data block of an
// implicit contract assertion whose check code generation inserts (the null
// dereference check) names the file and line and has a comment.
// p3100-violation-information-none.cpp is the same under =none.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/p3100-violation-information-full.C.

#include <contracts>
#include <cstdlib>
#include <cstring>

using namespace std::contracts;

void handle_contract_violation(const contract_violation &v) {
  if (std::strcmp(v.location().file_name(), __FILE__) != 0 ||
      !std::strstr(v.location().function_name(), "main") ||
      v.location().line() != 35 ||
      std::strcmp(v.comment(), "null pointer dereference") != 0 ||
      v.message() != nullptr || v.kind() != assertion_kind::implicit ||
      v.semantic() != evaluation_semantic::noexcept_observe)
    __builtin_abort();
  // The load would fault after an observed violation.
  std::_Exit(0);
}

int main() {
  int *volatile p = nullptr;
  volatile int r = *p; // line 35
  (void)r;
  __builtin_abort();
}
