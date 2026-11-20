// RUN: not %clang_cc1 -std=c++26 -fcontracts -fsyntax-only %s 2>&1 | FileCheck %s

// A contract specifier between a class's name and its body is not part of
// any function declaration.  It is diagnosed as such, in every form a
// specifier takes -- a plain predicate, an attribute before the predicate,
// a label -- with no special lookahead for it in the class head (CLANG-291).
//
// No GCC mirror: the recovery is Clang's.

// CHECK: :[[@LINE+1]]:17: error: 'pre' can only appear on a function declaration
struct S1 final pre(true) { };
// CHECK: :[[@LINE+1]]:17: error: 'pre' can only appear on a function declaration
struct S2 final pre [[maybe_unused]] (true) { };
// CHECK: :[[@LINE+1]]:17: error: 'pre' can only appear on a function declaration
struct S3 final pre<0>(true) { };
