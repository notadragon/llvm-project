// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3850 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p4298 %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p4405 %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fno-contracts -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fno-contracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=NOLINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3850 -fno-contracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK

// P3100 routes a sanitizer's reports to the contract-violation handler in C
// too, so a C link with -fcontracts-p3100 (or -fcontracts-p3850, which implies
// it) and a sanitizer links the contracts runtime, without -fcontracts-p4299.
// -fcontracts-p3100, -fcontracts-p4298 and -fcontracts-p4405 also turn C
// contracts (-fcontracts-p4299) on (DECISIONS.md E177, CLANG-885), so each
// links it on its own, as GCC's driver does; -fno-contracts, the C++ option,
// does not turn them off.  Nothing is linked when -fno-contracts-p3100 turns
// P3100 off, unless -fcontracts-p3850 turns C contracts on itself (E179 (1)).
// The routing itself is Runnable/p3100-c-routing.c.
//
// No GCC mirror: the link line is the only observable, which a DejaGnu test
// cannot inspect.

// LINK: "-lcontracts"
// NOLINK-NOT: "-lcontracts"

int main(void) { return 0; }
