// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-caller-unimplemented-warn-1.json -fsyntax-only -verify=warn %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-caller-unimplemented-warn-2.json -fsyntax-only -verify=warn %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-caller-unimplemented-warn-3.json -fsyntax-only -verify=nowarn %s

// P3595: Clang accepts (Task 1) but does not yet implement caller-side
// contract checking. When the effective configuration actually requests
// caller-side checking (a "caller" match with a non-"ignore" semantic),
// warn that it will not be honored. The default "caller-side entries are
// ignored unless matched otherwise" configuration must NOT warn.  The
// warning describes the configuration, so it carries no source location
// (matched with @*:*).

// warn-warning@*:* {{caller-side contract checking is configured}}

// nowarn-no-diagnostics

void f(int x) pre(x > 0) { }
