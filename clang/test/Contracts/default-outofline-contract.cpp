// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// An out-of-line explicitly defaulted function whose defaulting declaration
// repeats the class-scope contract is valid; one that adds a contract to a
// constructor is ill-formed.  The destructor form, which Clang does not
// compare (CLANG-544), is OpenBugs/dtor-redecl-contracts-not-compared.cpp.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/default-outofline-contract.C
// and default-outofline-adds-contract.C

struct S {
  int i = 0;
  S &operator=(const S &) pre(i >= 0);
};
S &S::operator=(const S &) pre(i >= 0) = default;

struct T { T(); }; // expected-note {{previously declared without contracts here}}
T::T() pre(true) = default; // expected-error {{method out-of-line definition differs in contract specifier sequence}}
