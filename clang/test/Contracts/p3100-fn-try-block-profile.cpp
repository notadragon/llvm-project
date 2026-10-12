// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -fcontracts -fcontracts-p3100 -fprofile-instrument=clang -fcoverage-mapping -emit-llvm %s -o - | FileCheck %s

// Under -fcontracts-p3100 a value-returning function's function-try-block is
// emitted with the flow-off check inside its EH scope, and still gets what
// EmitFunctionBody gives any body: the body's profile counter (counter 1 of g) is incremented.
//
// Mirror: gcc/testsuite/g++.dg/gcov/p3100-fn-try-block-coverage.C

int g(int a, int b) try {
  if (a && b)
    return 1;
  return 2;
} catch (...) {
  return 3;
}

// CHECK-LABEL: define {{.*}}@_Z1gii(
// CHECK: store i64 {{.*}}, ptr getelementptr inbounds (i8, ptr @__profc__Z1gii, i64 8)
// CHECK-LABEL: define {{.*}}@main(
int main() { return g(1, 0) == 2 ? 0 : 1; }
