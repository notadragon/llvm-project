// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// Basic P3400 label syntax on pre/post/contract_assert.

struct my_label_t {
  using assertion_control_object = my_label_t;
};
constexpr my_label_t my_label{};

void f(int x) pre<my_label>(x > 0) {}

int g(int x) post<my_label>(r: r >= 0) { return x; }

void h() {
  contract_assert<my_label>(true);
}

// Constexpr function returning a label
constexpr auto get_label() { return my_label_t{}; }
void k(int x) pre<get_label()>(x > 0) {}

// Direct constructor
void m(int x) pre<my_label_t{}>(x > 0) {}

// Variable template (exercises >> split)
template<typename T> constexpr my_label_t typed{};
void n(int x) pre<typed<int>>(x > 0) {}

// Multiple contracts, some with labels, some without
void mixed(int x)
  pre<my_label>(x > 0)
  pre(x < 100)
{}
