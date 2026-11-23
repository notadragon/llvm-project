// RUN: not %clang_cc1 -fcontracts-p4299 '-fcontract-configuration=[{"output":{"semantic":"enforce","x":1}}]' -fsyntax-only -fno-caret-diagnostics %s 2>&1 | FileCheck %s

// P3595 in C: c-config-diagnostic-main-file-location.c for the inline form,
// -fcontract-configuration=: here the unknown-key warning.  The main file is
// not "included from" <command-line> (no include context between the
// configuration's diagnostic and the next one, __INCLUDE_LEVEL__ 0), and the
// source's diagnostics are at their own lines in the main file.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-config-diagnostic-main-file-location-inline.c

// CHECK: {{^}}warning: unknown key 'x' in 'output' object in contract configuration from <command-line>
// CHECK-NEXT: {{^}}{{.*}}c-config-diagnostic-main-file-location-inline.c:18:2: warning: "located"
// CHECK-NEXT: {{^}}{{.*}}c-config-diagnostic-main-file-location-inline.c:20:9: error: expected expression
// CHECK-NOT: error:

_Static_assert(__INCLUDE_LEVEL__ == 0, "main file");

#warning "located"

int x = ;
