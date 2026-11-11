// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: not --crash %t | FileCheck %s

// P3098: an exception during capture initialization under enforce is a
// post_capture / evaluation_exception violation; the handler runs and, when
// it returns, the program terminates.  The exception must not escape f.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-except-init-enforce.C

#include <contracts>
#include <cstdio>
#include <cstdlib>

using namespace std::contracts;

void handle_contract_violation(const contract_violation &v) {
  if (v.kind() == assertion_kind::post_capture &&
      v.semantic() == evaluation_semantic::enforce &&
      v.detection_mode() == detection_mode::evaluation_exception)
    std::printf("HANDLER post_capture enforce evaluation_exception\n");
  else
    std::printf("HANDLER wrong violation\n");
  std::fflush(stdout);
}

struct Bad {
  Bad(int) { throw 42; }
  ~Bad() {}
};

int f(int i) post [b = Bad(1)] (true) { return i; }

int main() {
  try {
    f(10);
  } catch (...) {
    std::printf("ESCAPED\n");
    std::exit(0);
  }
  // Not reached: enforce terminates after the handler returns.
  std::printf("RETURNED\n");
  return 0;
}

// CHECK: HANDLER post_capture enforce evaluation_exception
// CHECK-NOT: ESCAPED
// CHECK-NOT: RETURNED
