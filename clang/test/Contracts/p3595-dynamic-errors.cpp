// P3595: output.dynamic validation errors.  Each of these aborts before the
// source file is lexed, so -verify cannot be used (see p3595-config-errors.cpp);
// checked with FileCheck instead.
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-err-noname.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=NONAME %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-err-pw.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=PW %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-err-empty.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=EMPTY %s
// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-err-linkage.json -fsyntax-only %s 2>&1 | FileCheck --check-prefix=LINKAGE %s

// NONAME: error: 'dynamic' in entry 0 of contract configuration from {{.*}} is missing required 'name' field
// PW: error: 'output' in entry 0 of contract configuration from {{.*}} requests 'provideweak' for 'dynamic' but has no 'semantic' field
// EMPTY: error: 'output' in entry 0 of contract configuration from {{.*}} must have a 'semantic' or a 'dynamic' field
// LINKAGE: error: invalid 'dynamic' linkage 'Pascal' in entry 0 of contract configuration from {{.*}}

void f(int x) pre(x > 0) { }
