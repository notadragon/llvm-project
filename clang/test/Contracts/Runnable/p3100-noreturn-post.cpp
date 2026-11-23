// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-invalid-noreturn \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:observe \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-invalid-noreturn \
// RUN:   -fcontract-evaluation-semantic=observe -fcontracts-p3098 -DCAPTURE \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:observe \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a [[noreturn]] function that returns
// (ub:dcl.attr.noreturn.eventually.returns) and has a postcondition: the
// postcondition is evaluated first, at the return, and then the [[noreturn]]
// check reports and, under "observe", aborts -- as GCC orders them.  Both
// where the epilogue evaluates the postcondition and where a postcondition
// capture makes a cleanup evaluate it.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-noreturn-post.C and
// p3100-noreturn-post-capture.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d comment=[%s]\n", (int)v.kind(), v.comment());
  std::fflush(stdout);
}
#ifdef CAPTURE
[[noreturn]] __attribute__((noinline)) void nr(const int x)
    post [y = x] (y < 0) {
#else
[[noreturn]] __attribute__((noinline)) void nr(const int x) post(x < 0) {
#endif
  if (x < 0)
    throw x;
}
void (*volatile fp)(int) = nr;
int main() {
  fp(1);
  std::printf("RESULT=1\n");
  return 0;
}
// CHECK: VIOL kind={{[0-9]+}} comment=[{{.*}}< 0]
// CHECK-NEXT: VIOL kind=7 comment=[a {{\[\[}}noreturn{{\]\]}} function returned]
// CHECK-NOT: RESULT
