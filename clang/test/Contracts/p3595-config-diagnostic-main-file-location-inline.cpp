// RUN: not %clang_cc1 -std=c++26 -fcontracts '-fcontract-configuration=[{"output":{"semantic":"enforce","x":1}}]' -fsyntax-only -fno-caret-diagnostics %s 2>&1 | FileCheck %s

// P3595: p3595-config-diagnostic-main-file-location.cpp for the inline form,
// -fcontract-configuration=: here the unknown-key warning.  The main file is
// not "included from" <command-line> (no include context between the
// configuration's diagnostic and the next one, __INCLUDE_LEVEL__ 0), and the
// source's diagnostics are at their own lines in the main file.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-config-diagnostic-main-file-location-inline.C

// CHECK: {{^}}warning: unknown key 'x' in 'output' object in contract configuration from <command-line>
// CHECK-NEXT: {{^}}{{.*}}p3595-config-diagnostic-main-file-location-inline.cpp:18:2: warning: "located"
// CHECK-NEXT: {{^}}{{.*}}p3595-config-diagnostic-main-file-location-inline.cpp:20:9: error: expected expression
// CHECK-NOT: error:

static_assert(__INCLUDE_LEVEL__ == 0);

#warning "located"

int x = ;
