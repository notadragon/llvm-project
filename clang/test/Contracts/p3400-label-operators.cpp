// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// P3400: a parenthesized comparison produces a bool, not an
// assertion_control_object, so it must be rejected as a label.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-label-operators.C)
//
// (Previously BUG-16: Clang did not validate that an assertion-control label is
// an assertion_control_object -- a bool-valued expression was accepted as a
// label.  Clang now requires the label to be a class type with an
// 'assertion_control_object' member type.)

struct weird_label_t {
  using assertion_control_object = weird_label_t;
  constexpr bool operator<(const weird_label_t&) const { return false; }
  constexpr bool operator>(const weird_label_t&) const { return true; }
};
constexpr weird_label_t weird_label;

// Unparenthesized use of a valid label works normally.
void f(int x) pre<weird_label>(x > 0) { }

// Parenthesized comparison -> bool, not a label.
void g(int x)
  pre<(weird_label_t{} < weird_label_t{})>(x > 0) // expected-error {{value of type 'bool' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
{
}

void h(int x)
  pre<(weird_label > weird_label)>(x > 0) // expected-error {{value of type 'bool' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
{
}
