// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// typeid of a glvalue of polymorphic class type evaluates its operand, so
// naming a local variable or parameter there odr-uses it, and a lambda with
// no capture-default must capture it ([expr.prim.lambda.capture]/7 captures
// regardless of typeid).  A non-polymorphic operand is unevaluated.  No
// contracts are involved; it bears on EDG-58's shape (typeid of a by-value
// polymorphic parameter in a postcondition odr-uses it).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-typeid-polymorphic-capture.C,
// which GCC trunk fails (GCC-644, stock: it accepts both).

namespace std { class type_info; }

struct B { virtual ~B(); };
struct N { };

void f(B x) { // expected-note {{declared here}}
  auto l = [] { return &typeid(x); }; // expected-error {{variable 'x' cannot be implicitly captured in a lambda with no capture-default specified}} \
                                      // expected-note {{lambda expression begins here}} expected-note 4 {{capture}}
}

void g() {
  B y; // expected-note {{declared here}}
  auto l = [] { return &typeid(y); }; // expected-error {{variable 'y' cannot be implicitly captured in a lambda with no capture-default specified}} \
                                      // expected-note {{lambda expression begins here}} expected-note 4 {{capture}}
}

void h(N z, B &r) {
  auto l = [] { return &typeid(z); };   // ok, unevaluated
  auto m = [&] { return &typeid(r); };  // ok, captured
}
