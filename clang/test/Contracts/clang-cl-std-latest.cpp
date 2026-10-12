// RUN: %clang_cl --target=x86_64-pc-windows-msvc /std:c++latest /Zs -Xclang -verify -- %s
// expected-no-diagnostics

// clang-cl /std:c++latest selects the newest standard and enables
// contracts, so a precondition parses.  The C-input case is
// OpenBugs/c-fcontracts-no-warning.c.
//
// No GCC mirror: GCC has no cl-compatible driver.

int f(int x) pre(x > 0) { return x; }
