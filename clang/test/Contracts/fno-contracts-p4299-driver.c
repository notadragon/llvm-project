// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p4299 -fno-contracts-p4299 %s 2>&1 | FileCheck %s

// -fcontracts-p4299 -fno-contracts-p4299 leaves C contracts off, so the Gnu
// toolchain's link step adds no -lcontracts.  The C++ shape is
// fno-contracts-subflag-driver.cpp.
//
// No GCC mirror: the link line is the only observable, which a DejaGnu test
// cannot inspect.

// CHECK-NOT: "-lcontracts"

int main(void) { return 0; }
