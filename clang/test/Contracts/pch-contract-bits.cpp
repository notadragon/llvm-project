// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -x c++-header %t/h.h -emit-pch -o %t/h.pch
// RUN: %clang_cc1 -std=c++26 -fcontracts -include-pch %t/h.pch -fsyntax-only -verify %t/use.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -include-pch %t/h.pch -fsyntax-only -ast-dump -ast-dump-filter f %t/dump.cpp | FileCheck %s

// The contract state Sema stores on existing nodes -- FieldDecl
// IsConstifiedCapture, LambdaCapture CapturedAcrossContract and ContractLoc
// -- survives a PCH: a template instantiated from a PCH keeps the
// contract-specific diagnostic, and a by-reference capture in a predicate
// keeps its const type.
//
// No GCC mirror: the bits are Clang AST state with no GCC counterpart.

//--- h.h
template <class T> int h(T x) pre(++x > 0) { return x; }
inline int f(int x) pre([&] { return x > 0; }()) { return x; }

//--- use.cpp
// expected-error@h.h:1 {{cannot assign to variable 'x' because it is considered 'const' inside of a contract}}
int use() { return h(1); } // expected-note {{in instantiation of function template specialization}}

//--- dump.cpp
int g() { return f(1); }

// CHECK: FunctionDecl {{.*}} f 'int (int)'
// CHECK: DeclRefExpr {{.*}} 'const int' lvalue ParmVar {{.*}} 'x' 'int'
