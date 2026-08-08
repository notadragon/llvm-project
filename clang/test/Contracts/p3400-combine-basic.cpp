// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// Basic operator| combining: __combined_label satisfies
// assertion_control_object and can be chained.

namespace my {
  struct A { using assertion_control_object = A; };
  struct B { using assertion_control_object = B; };
  struct C { using assertion_control_object = C; };
  constexpr A a{};
  constexpr B b{};
  constexpr C c{};
}

using contract_control namespace my;

namespace std::contracts::labels {
  template<typename L, typename R>
    requires requires { typename L::assertion_control_object;
                        typename R::assertion_control_object; }
  struct __combined_label {
    using assertion_control_object = __combined_label;
    constexpr __combined_label(const L& l, const R& r) : _M_lhs(l), _M_rhs(r) {}
    constexpr __combined_label(const __combined_label&) = default;
    L _M_lhs;
    R _M_rhs;
  };

  template<typename L, typename R>
    requires requires { typename L::assertion_control_object;
                        typename R::assertion_control_object; }
  constexpr auto operator|(const L& l, const R& r)
  { return __combined_label<L, R>(l, r); }
}

using contract_control namespace std::contracts::labels;

// Basic combine
constexpr auto ab = contract_control(a | b);
static_assert(requires { typename decltype(ab)::assertion_control_object; });

// Chaining
constexpr auto abc = contract_control(a | b | c);
static_assert(requires { typename decltype(abc)::assertion_control_object; });

// In label expressions
void f(int x) pre<contract_control(a | b)>(x > 0) {}
void g(int x) pre<contract_control(a | b | c)>(x > 0) {}
