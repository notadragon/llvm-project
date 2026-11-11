// P4405: -fsanitize-semantic= rejects prove and diagnose, for a routed and
// for a non-routed sanitizer check alike, with and without
// -fcontracts-p4405: no P3100 implicit assertion allows them.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-sanitize-semantic.C and
// p4405-sanitize-semantic-no-flag.C

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4405 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:prove %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ASAN-PROVE
// ASAN-PROVE: error: '-fsanitize-semantic=address:prove' is not supported

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4405 -fsanitize=return \
// RUN:   -fsanitize-semantic=return:diagnose %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=RETURN-DIAGNOSE
// RETURN-DIAGNOSE: error: '-fsanitize-semantic=return:diagnose' is not supported

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=undefined -fsanitize-semantic=undefined:prove %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=UB-PROVE
// UB-PROVE: error: '-fsanitize-semantic=undefined:prove' is not supported
