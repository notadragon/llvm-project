// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags -S -emit-llvm -o - %s | FileCheck %s

// The violation arm of a C contract check is marked unlikely, under every
// semantic that checks (notadragon_wg21 DECISIONS-resolved.md E63 (a)): one
// branch with likely weights per check -- here a precondition, a
// postcondition and a contract assertion under observe.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-check-unlikely.c

#include <contracts.h>

int f(int x) _Pre(x > 0) _Post(r : r > 0) {
  _ContractAssert(x != 3);
  return x;
}

// CHECK-COUNT-3: br i1 {{.*}}, label %{{.*}}, label %{{.*}}, !prof ![[#]]
// CHECK-NOT: !prof
// CHECK: = !{!"branch_weights", {{.*}}i32 {{[0-9]+}}, i32 1}
// CHECK-NOT: branch_weights

// REQUIRES: contracts-libcxx
