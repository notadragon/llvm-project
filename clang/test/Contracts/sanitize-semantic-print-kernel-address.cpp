// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=kernel-address -Xclang -fsanitize-semantic-print \
// RUN:   -o /dev/null %s 2>&1 | FileCheck --allow-empty %s

// -fsanitize=kernel-address enables no user-space address check, so
// -fsanitize-semantic-print prints no "address:" line.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/sanitize-semantic-print-kernel-address.C

// CHECK-NOT: {{^}}address:

int main() { return 0; }
