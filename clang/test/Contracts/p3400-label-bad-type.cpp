// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// P3400: a label whose type lacks assertion_control_object (or is not a class
// type at all) must be rejected.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-label-bad-type.C)
//
// (Previously: Clang did not validate the assertion-control label type --
// a struct without assertion_control_object, a struct with a non-type member of
// that name, and an integer literal were all accepted as labels.  Clang now
// requires the label to be a class type with an 'assertion_control_object'
// member type.)

struct not_a_label {};
constexpr not_a_label bad_label;

struct also_bad {
  int assertion_control_object = 0;  // not a type
};
constexpr also_bad bad_label2{};

void f(int x) pre<bad_label>(x > 0) // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
{
}

void g(int x) pre<bad_label2>(x > 0) // expected-error {{value of type 'const also_bad' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
{
}

void h() {
  contract_assert<42>(true); // expected-error {{value of type 'int' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
}
