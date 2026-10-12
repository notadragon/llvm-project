// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3400: a variable-template label with a trailing >> (rshift split) parses in
// assertion-control expressions.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-label-rshift.C)

struct empty_label_t {
  using assertion_control_object = empty_label_t;
};

template<typename T>
constexpr empty_label_t typed_label{};

void f(int x) pre<typed_label<int>>(x > 0) { }

int g(int x) post<typed_label<int>>(r: r >= 0) { return x; }

void h() { contract_assert<typed_label<int>>(true); }
