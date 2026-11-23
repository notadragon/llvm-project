// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -fcontracts -fcontracts-p3100 -fprofile-instrument=clang -fcoverage-mapping -fcoverage-mcdc -emit-llvm %s -o - | FileCheck %s

// Under -fcontracts-p3100 a value-returning function's function-try-block is
// emitted with the flow-off check inside its EH scope, and still gets what
// EmitFunctionBody gives any body: g gets its MC/DC condition bitmap.
//
// Mirror: gcc/testsuite/g++.dg/gcov/p3100-fn-try-block-coverage.C (GCC has no
// MC/DC form of this; it pins the coverage).

int g(int a, int b) try {
  if (a && b)
    return 1;
  return 2;
} catch (...) {
  return 3;
}

// CHECK-LABEL: define {{.*}}@_Z1gii(
// CHECK: %mcdc.addr = alloca i32
// CHECK-LABEL: define {{.*}}@main(
int main() { return g(1, 0) == 2 ? 0 : 1; }
