// RUN: %clang_cc1 -round-trip-args -std=c++26 -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:noexcept_enforce \
// RUN:   -fsanitize-semantic-print -fsyntax-only %s 2>&1 | FileCheck %s
// RUN: %clang_cc1 -no-round-trip-args -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:noexcept_enforce \
// RUN:   -fsanitize-semantic-print -fsyntax-only %s 2>&1 | FileCheck %s

// -fsanitize-semantic-print prints each check's semantic once, also when the
// arguments make a round trip (the default on an assertions build), which
// parses them twice (CLANG-139: it printed while parsing them).

// CHECK: {{^}}address: noexcept_enforce
// CHECK-NOT: address:

int main() { return 0; }
