// RUN: not %clang_cc1 -fcontracts-p4299 -fcontract-configuration-file=%S/c-config-diagnostic-main-file-location.json -fsyntax-only -fno-caret-diagnostics %s 2>&1 | FileCheck %s

// P3595 in C: a diagnostic about the contract configuration -- here the
// unknown-key warning -- does not change how the main file is entered: the
// main file is not "included from" the configuration (no include context
// between the configuration's diagnostic and the next one,
// __INCLUDE_LEVEL__ 0), and the source's diagnostics are at their own lines
// in the main file.  The CHECK-NEXT chain takes the configuration's
// diagnostic, the #warning and the error together, so any line between them
// fails it.  Clang's configuration diagnostics carry no source location.
// The inline form: c-config-diagnostic-main-file-location-inline.c.  For
// C++: p3595-config-diagnostic-main-file-location*.cpp.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-config-diagnostic-main-file-location.c

// CHECK: {{^}}warning: unknown key 'module' in 'match' object in contract configuration from {{.*}}c-config-diagnostic-main-file-location.json
// CHECK-NEXT: {{^}}{{.*}}c-config-diagnostic-main-file-location.c:23:2: warning: "located"
// CHECK-NEXT: {{^}}{{.*}}c-config-diagnostic-main-file-location.c:25:9: error: expected expression
// CHECK-NOT: error:

_Static_assert(__INCLUDE_LEVEL__ == 0, "main file");

#warning "located"

int x = ;
