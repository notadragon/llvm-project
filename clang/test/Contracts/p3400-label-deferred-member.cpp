// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// P3400: a label's structural requirement is checked for member functions,
// whose predicates are deferred, as for free functions: in-class
// definitions, in-class declarations and their out-of-line redeclarations,
// and members of class templates.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-label-deferred-member.C, GCC-198)

struct not_a_label { int x; };
constexpr not_a_label bad_lbl{};

void freefn(int x) pre<bad_lbl>(x > 0) { } // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}

struct S {
  void memfn(int x) pre<bad_lbl>(x > 0) { } // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
  int post_fn(int x) post<bad_lbl>(r: r > 0) { return x; } // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
};

struct T {
  void memdecl(int x) pre<bad_lbl>(x > 0); // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
  int post_decl(int x) post<bad_lbl>(r: r > 0); // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
};

void T::memdecl(int x) pre<bad_lbl>(x > 0) { } // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
int T::post_decl(int x) post<bad_lbl>(r: r > 0) { return x; } // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}

// A member of a class template: Clang checks a non-dependent label when the
// member is instantiated (GCC checks it at definition time; either is
// conforming).
template<class U>
struct TT {
  void m(U x) pre<bad_lbl>(x > 0); // expected-error {{value of type 'const not_a_label' is not a valid assertion-control label; its type must be a class type with an 'assertion_control_object' member type}}
};
void use(TT<int> t) {
  t.m(1); // expected-note {{in instantiation of member function 'TT<int>::m' requested here}}
}
