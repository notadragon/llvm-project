// RUN: %clang_cc1 -std=c++26 -fcontracts -ast-print %s | FileCheck %s --strict-whitespace

// -ast-print lays out a contract_assert as a statement of its own: indented
// like its neighbours and followed by a newline.
//
// No GCC mirror: the AST printer has no GCC counterpart.

int h(int x) { contract_assert(x != 0); return x; }

// CHECK:      {{^}}int h(int x) {{[{]}}
// CHECK-NEXT: {{^}}    contract_assert(x != 0);{{$}}
// CHECK-NEXT: {{^}}    return x;{{$}}
