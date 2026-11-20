// DEFINE: %{cxx} = %clangxx -std=c++26 -fcontracts-p4451 -fcontract-evaluation-semantic=observe %libcxx_flags
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %{cxx} -x c++-header %t/h.h -o %t/h.pch
// RUN: %{cxx} -include-pch %t/h.pch -fsyntax-only -Xclang -ast-dump \
// RUN:   -Xclang -ast-dump-filter -Xclang dflt_ %t/use.cpp | FileCheck %s
// RUN: %{cxx} -include %t/h.h -fsyntax-only -Xclang -ast-dump \
// RUN:   -Xclang -ast-dump-filter -Xclang dflt_ %t/use.cpp | FileCheck %s
// RUN: %{cxx} -include-pch %t/h.pch -fsyntax-only -Xclang -verify %t/redecl.cpp
// REQUIRES: contracts-libcxx

// P4451R3: the bit an assertion-control-specifier naming
// std::contracts::controls::defaulted leaves on a contract assertion
// survives a PCH: the dump shows it beside the configured semantic it does
// not change (observe; enforced's assertion is adjusted to enforce), on a
// template's instantiation in the using file too, and redeclarations in the
// using file are matched against it.  The same dump without the PCH is the
// control.  -fcontracts-p4451 alone: under -fcontracts-p3400 the specifier
// is a label and leaves no bit.

//--- h.h
#include <contracts>

namespace scl = std::contracts::controls;

inline int dflt_pre(int x) pre<scl::defaulted>(x > 0) { return x; }
inline int dflt_assert(int x) {
  contract_assert<std::contracts::controls::defaulted>(x > 0);
  return x;
}
inline int dflt_none(int x) pre(x > 0) { return x; }
inline int dflt_enforced(int x) pre<scl::enforced>(x > 0) { return x; }
template <class T> T dflt_tmpl(T x) pre<scl::defaulted>(x > 0) { return x; }

int dflt_decl(int x) pre<scl::defaulted>(x > 0);
int dflt_decl_none(int x) pre(x > 0);
int dflt_decl_enforced(int x) pre<scl::enforced>(x > 0);

//--- use.cpp
int g() {
  return dflt_pre(1) + dflt_assert(1) + dflt_none(1) + dflt_enforced(1) +
         dflt_tmpl(1);
}

// CHECK: FunctionDecl {{.*}} dflt_pre 'int (int)'
// CHECK: ContractStmt {{.*}} pre observe defaulted{{$}}
// CHECK: FunctionDecl {{.*}} dflt_assert 'int (int)'
// CHECK: ContractStmt {{.*}} contract_assert observe defaulted{{$}}
// CHECK: FunctionDecl {{.*}} dflt_none 'int (int)'
// CHECK: ContractStmt {{.*}} pre observe{{$}}
// CHECK: FunctionDecl {{.*}} dflt_enforced 'int (int)'
// CHECK: ContractStmt {{.*}} pre enforce terminating{{$}}
// CHECK: FunctionDecl {{.*}} dflt_tmpl 'int (int)'
// CHECK: ContractStmt {{.*}} pre observe defaulted{{$}}

//--- redecl.cpp
int dflt_decl(int x) pre<scl::defaulted>(x > 0);
int dflt_decl(int x) pre<std::contracts::controls::defaulted>(x > 0) { return x; }
int dflt_decl_none(int x) pre<scl::defaulted>(x > 0); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
// expected-note@h.h:15 {{contract previously specified without an assertion-control-specifier}}
int dflt_decl_enforced(int x) pre<scl::defaulted>(x > 0); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
// expected-note@h.h:16 {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::enforced'}}
int dflt_pre(int x) pre(x > 0); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
// expected-note@h.h:5 {{contract previously specified with an assertion-control-specifier naming 'std::contracts::controls::defaulted'}}
