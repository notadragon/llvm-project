// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -fcontracts -emit-pch -o %t/def.ast %t/def.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -emit-obj -o /dev/null -ast-merge %t/def.ast %t/empty.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -ast-merge %t/def.ast %t/redecl.cpp

// ASTImporter imports a function's contracts -- the ContractSpecifierDecl, its
// ContractStmts, result names -- and a contract_assert in its body into the
// destination context.  Code generation of the merged function works, and
// redeclaring it with the same contracts is accepted.
//
// No GCC mirror: ASTImporter has no GCC counterpart.

//--- def.cpp
int f(int x) pre(x > 0) { return x; }
int h(const int x) post(r : r > x) {
  contract_assert(x != 3);
  return x + 1;
}

//--- empty.cpp
int g();

//--- redecl.cpp
// expected-no-diagnostics
int f(int x) pre(x > 0);
int h(const int x) post(r : r > x);
