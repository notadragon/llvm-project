// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3850 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 %s 2>&1 | FileCheck %s --check-prefix=NOLINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fno-contracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=NOLINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3850 -fno-contracts-p3100 -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=NOLINK
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts-p3100 -fno-contracts -fsanitize=address %s 2>&1 | FileCheck %s --check-prefix=NOLINK

// P3100 routes a sanitizer's reports to the contract-violation handler in C
// too, so a C link with -fcontracts-p3100 (or -fcontracts-p3850, which implies
// it) and a sanitizer links the contracts runtime, without -fcontracts-p4299,
// as GCC's driver does.  Without a sanitizer nothing is routed and nothing is
// linked; nor when -fno-contracts-p3100 or -fno-contracts turns P3100 off.
// The routing itself is Runnable/p3100-c-routing.c.
//
// No GCC mirror: the link line is the only observable, which a DejaGnu test
// cannot inspect.

// LINK: "-lcontracts"
// NOLINK-NOT: "-lcontracts"

int main(void) { return 0; }
