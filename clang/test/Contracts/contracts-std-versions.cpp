// Every standard at or above C++26 enables contracts, in both its year
// spelling and its provisional one.
//
// tools::wantsCxxContracts decides this by comparing the -std= value against
// a hardcoded list of spellings, and it is the sole gate: nothing in cc1
// keys contracts off the language standard, so an omission there means the
// front end never sees -fcontracts and a contract becomes a plain syntax
// error.  c++2d was omitted, so this compiled on GCC (which enables contracts
// at -std=c++29/-std=c++2d) and failed here.
//
// Driver level: -fcontracts must be forwarded to cc1 for each spelling.
// RUN: %clang -### -std=c++26 -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -std=gnu++26 -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -std=c++2c -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -std=gnu++2c -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -std=c++2d -c %s 2>&1 | FileCheck %s --check-prefix=ON
// RUN: %clang -### -std=gnu++2d -c %s 2>&1 | FileCheck %s --check-prefix=ON
//
// Below C++26 it must not be, and an explicit -fno-contracts must win.
// RUN: %clang -### -std=c++23 -c %s 2>&1 | FileCheck %s --check-prefix=OFF
// RUN: %clang -### -std=c++2d -fno-contracts -c %s 2>&1 | \
// RUN:   FileCheck %s --check-prefix=OFF
//
// ON: "-fcontracts"
// OFF-NOT: "-fcontracts"
//
// Front end: the contract actually parses at the newest spelling, which is
// what the driver test above cannot tell you on its own.
// RUN: %clang_cc1 -std=c++2d -fcontracts -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// expected-no-diagnostics

int f(int x) pre(x > 0) post(r: r > 0) { return x; }

int g(int x) {
  contract_assert(x != 0);
  return x;
}
