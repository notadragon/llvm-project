// RUN: %clang_cc1 -std=c++26 -fcontracts -ast-print -ast-dump-filter result %s | FileCheck %s

// DeclPrinter::VisitResultNameDecl prints the result-name declaration's name
// once (-ast-dump-filter hands each matching NamedDecl to Decl::print).
//
// No GCC mirror: DeclPrinter has no GCC counterpart.

int f(int x) post(result : result > 0) { return x; }

// CHECK: Printing result:
// CHECK-NEXT: {{^}}int result{{$}}
