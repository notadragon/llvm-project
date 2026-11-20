// RUN: %clang -### -std=c23 -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p3290 -fcontracts-p3400 -fcontracts-p4283 -fcontracts-p4301 -fcontracts-p4451 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=POS,CC1
// RUN: %clang -### -std=c23 -fno-contracts-p3400 -fno-contracts-p4451 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=NEG,CC1
// RUN: %clang -### -std=c23 -fcontracts-p3400 -fcontracts-p3100 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=MIXED
// RUN: %clang -### -x objective-c -fcontracts-p3400 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=OBJC,CC1
// RUN: %clang -### -x c++ -std=c++23 -fcontracts-p3400 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=CXX
// RUN: %clang -### -std=c23 -fcontracts -fcontract-bypass-rethrowing-local-handler -fcontract-analysis-skip-dead-calls -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=NONPAPER,CC1
// RUN: %clang -### -std=c23 -fno-contracts -fno-contract-bypass-rethrowing-local-handler -fno-contract-analysis-skip-dead-calls -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=NONPAPERNEG,CC1
// RUN: %clang -### -std=c23 -fcontracts-p4405 -fcontract-analysis-skip-dead-calls -fcontract-bypass-rethrowing-local-handler -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=NONPAPERMIXED

// DECISIONS.md E179 (3): a C++-only contracts per-paper option on a C input
// says it is valid for C++ but not for C, as GCC does, in either form; it is
// not passed on, so it neither turns contracts on nor draws the "argument
// unused" warning as well.  The C per-paper options keep working beside it.
// The same for -fcontracts itself and the C++-only non-paper options
// (DECISIONS.md E183 (1)).

// POS: warning: command-line option '-fcontracts-p3097' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p3098' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p3099' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p3290' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p3400' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p4283' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p4301' is valid for C++ but not for C
// POS-NEXT: warning: command-line option '-fcontracts-p4451' is valid for C++ but not for C

// NEG: warning: command-line option '-fno-contracts-p3400' is valid for C++ but not for C
// NEG-NEXT: warning: command-line option '-fno-contracts-p4451' is valid for C++ but not for C

// OBJC: warning: command-line option '-fcontracts-p3400' is valid for C++ but not for Objective-C

// NONPAPER: warning: command-line option '-fcontracts' is valid for C++ but not for C
// NONPAPER-NEXT: warning: command-line option '-fcontract-bypass-rethrowing-local-handler' is valid for C++ but not for C
// NONPAPER-NEXT: warning: command-line option '-fcontract-analysis-skip-dead-calls' is valid for C++ but not for C

// NONPAPERNEG: warning: command-line option '-fno-contracts' is valid for C++ but not for C
// NONPAPERNEG-NEXT: warning: command-line option '-fno-contract-bypass-rethrowing-local-handler' is valid for C++ but not for C
// NONPAPERNEG-NEXT: warning: command-line option '-fno-contract-analysis-skip-dead-calls' is valid for C++ but not for C

// CC1-NOT: warning:
// CC1: "-cc1"
// CC1-NOT: "-fcontracts{{.*}}"

// MIXED: warning: command-line option '-fcontracts-p3400' is valid for C++ but not for C
// MIXED-NOT: warning:
// MIXED: "-cc1"
// MIXED-NOT: "-fcontracts-p3400"
// MIXED-SAME: "-fcontracts-p4299" "-fcontracts-p3100"
// MIXED-NOT: "-fcontracts-p3400"

// NONPAPERMIXED: warning: command-line option '-fcontract-analysis-skip-dead-calls' is valid for C++ but not for C
// NONPAPERMIXED-NEXT: warning: command-line option '-fcontract-bypass-rethrowing-local-handler' is valid for C++ but not for C
// NONPAPERMIXED-NOT: warning:
// NONPAPERMIXED: "-cc1"
// NONPAPERMIXED-NOT: "-fcontract-analysis-skip-dead-calls"
// NONPAPERMIXED-NOT: "-fcontract-bypass-rethrowing-local-handler"
// NONPAPERMIXED-SAME: "-fcontracts-p4299"
// NONPAPERMIXED-SAME: "-fcontracts-p4405"
// NONPAPERMIXED-NOT: "-fcontract-analysis-skip-dead-calls"
// NONPAPERMIXED-NOT: "-fcontract-bypass-rethrowing-local-handler"

// CXX-NOT: warning:
// CXX: "-cc1"
// CXX-SAME: "-fcontracts"
// CXX-SAME: "-fcontracts-p3400"

int i;
