// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A lambda in the precondition of a non-defining templated declaration -- a
// namespace-scope function template, a member function template, a member
// of a class template -- with and without instantiation.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/template-decl-contract-lambda.C,
// template-decl-contract-lambda-class.C and
// template-decl-contract-lambda-nsscope.C

template <class T> void fa(T t) pre([&] { return t > 0; }());

struct SB {
  template <class T> void fb(T t) pre([&] { return t > 0; }());
};

template <class U> struct SC {
  void fc(U t) pre([t] { return t > 0; }());
};

void use(SB &b, SC<int> &c) {
  fa(1);
  b.fb(1);
  c.fc(1);
}
