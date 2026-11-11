// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s

// A lambda in a precondition or postcondition: what it captures by reference
// is const (also *this, through [this] or an implicit this), so only the
// copies of a mutable lambda can be modified; a lambda capturing a value
// parameter in a postcondition, implicitly or explicitly, odr-uses it, so it
// must be const (sizeof does not); and an implicit capture needs a
// capture-default, including from a nested lambda.  (EDG-1/2/39 shapes.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-declaration-entities-errors.C,
// which leaves out the two *this rows: GCC accepts them, a separate defect
// with its own watch test.

struct S {
  int m;
  void f() pre([this] { return ++m > 0; }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void g() pre([&] { return ++m > 0; }());    // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
};
void a(int x) pre([&] { return ++x > 0; }());            // expected-error {{cannot assign to a variable captured by reference which was captured as const}}
void b(int x) pre([&x] { return ++x > 0; }());           // expected-error {{cannot assign to a variable captured by reference which was captured as const}}
void c(int x) pre([x]() mutable { return ++x > 0; }());  // OK, a copy
void d(int x) pre([x] { return ++x > 0; }());            // expected-error {{cannot assign to a variable captured by copy in a non-mutable lambda}}
int e() post(r : [&r] { return ++r > 0; }());            // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
int e2() post(r : [r]() mutable { return ++r > 0; }());  // OK, a copy
int p(int x) post([&] { return x > 0; }());              // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}} expected-note {{parameter of type 'int' is declared here}}
int p2(int x) post([x] { return x > 0; }());             // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}} expected-note {{parameter of type 'int' is declared here}}
int p3(int x) post([&] { return sizeof(x) > 0; }());     // OK, unevaluated
int p4(const int x) post([&] { return x > 0; }());       // OK
void n(int x) pre([] { return x > 0; }());               // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}} expected-note {{'x' declared here}} expected-note {{lambda expression begins here}} expected-note 2 {{capture 'x' by}} expected-note 2 {{default capture by}}
void n2(int x) pre([=] { return [] { return x > 0; }(); }()); // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}} expected-note {{'x' declared here}} expected-note {{lambda expression begins here}} expected-note 2 {{capture 'x' by}} expected-note 2 {{default capture by}}
