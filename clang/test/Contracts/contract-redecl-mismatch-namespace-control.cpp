// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// The namespace-scope control for contract-friend-deferred-mismatch.cpp:
// the mismatch between two friend declarations (CLANG-12, fixed) is
// diagnosed at namespace scope too, with the same diagnostic, which kept that
// gap on the friend path rather than on contract matching while it was
// open.

// expected-note@+1 {{contract previously specified with a non-equivalent condition}}
int g(int x) pre(x > 0);
// expected-error@+2 {{function redeclaration differs in contract specifier sequence}}
// expected-note@+1 {{in contract specified here}}
int g(int x) pre(x < 0);
