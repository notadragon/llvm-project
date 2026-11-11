// RUN: %clang -### -std=c23 -fcontracts-p3850 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=CHECK,ALL
// RUN: %clang -### -std=c23 -fcontracts-p3850 -fno-contracts-p4298 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=CHECK,NO4298
// RUN: %clang -### -std=c23 -fno-contracts-p3100 -fno-contracts-p4298 -fno-contracts-p4405 -fcontracts-p3850 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --check-prefixes=CHECK,NONE

// DECISIONS.md E179 (1): on a C input -fcontracts-p3850 implies the C
// per-paper options, except one explicitly disabled, and turns C contracts on
// itself, even with all three disabled.  -cc1 gets the resolved C options,
// never the C++ -fcontracts or the umbrella.

// CHECK-NOT: warning:
// CHECK: "-cc1"
// CHECK-NOT: "-fcontracts"
// CHECK-NOT: "-fcontracts-p3850"
// CHECK-SAME: "-fcontracts-p4299"
// ALL-SAME: "-fcontracts-p3100" "-fcontracts-p4298" "-fcontracts-p4405"
// NO4298-SAME: "-fcontracts-p3100" "-fcontracts-p4405"
// NONE-NOT: "-fcontracts-p{{3100|4298|4405}}"
// CHECK-NOT: "-fcontracts"
// CHECK-NOT: "-fcontracts-p3850"

int i;
