// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-config-diagnostic-main-file-location.json -fsyntax-only -fno-caret-diagnostics %s 2>&1 | FileCheck %s

// P3595: a diagnostic about the contract configuration -- here the
// unknown-key warning -- does not change how the main file is entered: the
// main file is not "included from" the configuration (no include context
// between the configuration's diagnostic and the next one,
// __INCLUDE_LEVEL__ 0), and the source's diagnostics are at their own lines
// in the main file.  The CHECK-NEXT chain takes the configuration's
// diagnostic, the #warning and the error together, so any line between them
// fails it.  Clang's configuration diagnostics carry no source location.
// The inline form: p3595-config-diagnostic-main-file-location-inline.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-diagnostic-main-file-location.C

// CHECK: {{^}}warning: unknown key 'module' in 'match' object in contract configuration from {{.*}}p3595-config-diagnostic-main-file-location.json
// CHECK-NEXT: {{^}}{{.*}}p3595-config-diagnostic-main-file-location.cpp:22:2: warning: "located"
// CHECK-NEXT: {{^}}{{.*}}p3595-config-diagnostic-main-file-location.cpp:24:9: error: expected expression
// CHECK-NOT: error:

static_assert(__INCLUDE_LEVEL__ == 0);

#warning "located"

int x = ;
