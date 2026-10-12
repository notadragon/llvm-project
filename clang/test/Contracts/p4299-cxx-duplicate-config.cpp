// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 -fcontracts-p4299 -fcontract-evaluation-semantic=observe -c %s 2>&1 | FileCheck --check-prefix=EVAL %s
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 -fcontracts-p4299 -fcontract-configuration-file=x.json -c %s 2>&1 | FileCheck --check-prefix=FILE %s
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c++26 -fcontracts-p4299 -fcontract-group-evaluation-semantic=g:observe -c %s 2>&1 | FileCheck --check-prefix=GROUP %s
// RUN: %clang -### --target=x86_64-unknown-linux-gnu -x c -std=c23 -fcontracts-p4299 -fcontract-evaluation-semantic=observe -c %s 2>&1 | FileCheck --check-prefix=EVAL %s

// -fcontracts-p4299 in a C++ compile renders each contract configuration
// option to cc1 once, not once from the C++ contracts block and again from the
// P4299 block; a C compile with -fcontracts-p4299 renders it once too.
//
// No GCC mirror: GCC's driver does not re-render these options, and a
// duplicated diagnostic cannot be observed from DejaGnu (one dg-warning
// prunes every repeat on its line).

// EVAL: "-fcontract-evaluation-semantic=observe"
// EVAL-NOT: "-fcontract-evaluation-semantic=

// FILE: "-fcontract-configuration-file=x.json"
// FILE-NOT: "-fcontract-configuration-file=

// GROUP: "-fcontract-group-evaluation-semantic=g:observe"
// GROUP-NOT: "-fcontract-group-evaluation-semantic=

int f(int x) pre(x > 0) { return x; }
