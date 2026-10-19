// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxABCDEFGHIJ \
// RUN:   %s -### 2>&1 | FileCheck %s

// An unknown -fsanitize-semantic= semantic name is quoted whole, however long
// (the name below is 70 characters).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/sanitize-semantic-long-name.C

// CHECK: unknown contract evaluation semantic '{{x+}}ABCDEFGHIJ'

int main() { return 0; }
