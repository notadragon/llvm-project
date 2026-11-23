// The driver passes -fcontract-violation-information= to -cc1 in every
// language, whatever the other contracts options, and -cc1 predefines
// __clang_contracts_violation_information_none under =none only.
//
// RUN: %clang -### -fcontract-violation-information=none -c %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DRIVER
// RUN: %clang -### -x c++ -std=c++17 -fcontract-violation-information=none \
// RUN:   -c %s 2>&1 | FileCheck %s --check-prefix=DRIVER
// DRIVER: "-cc1"
// DRIVER-SAME: "-fcontract-violation-information=none"
//
// RUN: %clang -fcontract-violation-information=none \
// RUN:   -Werror=unused-command-line-argument -E -dM %s \
// RUN:   | FileCheck %s --check-prefix=NONE
// RUN: %clang -x c++ -std=c++26 -fcontract-violation-information=none \
// RUN:   -E -dM %s | FileCheck %s --check-prefix=NONE
// RUN: %clang -x objective-c -fcontract-violation-information=none \
// RUN:   -E -dM %s | FileCheck %s --check-prefix=NONE
// RUN: %clang -x objective-c++ -fcontract-violation-information=none \
// RUN:   -E -dM %s | FileCheck %s --check-prefix=NONE
// RUN: %clang -fcontracts-p4299 -fcontract-violation-information=full \
// RUN:   -E -dM %s | FileCheck %s --check-prefix=FULL
// RUN: %clang -x c++ -std=c++26 -fcontracts-p3850 -E -dM %s \
// RUN:   | FileCheck %s --check-prefix=FULL
// RUN: %clang -x c++ -std=c++26 -fcontract-violation-information=none \
// RUN:   -fcontract-violation-information=full -E -dM %s \
// RUN:   | FileCheck %s --check-prefix=FULL
// NONE: #define __clang_contracts_violation_information_none 1
// FULL-NOT: __clang_contracts_violation_information_none
//
// RUN: not %clang -fcontract-violation-information=some -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD
// BAD: error: invalid value 'some' in '-fcontract-violation-information=some'
