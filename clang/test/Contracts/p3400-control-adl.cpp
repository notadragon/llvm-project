// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// P3400: free-function operators found through a `using contract_control`
// directive.  (GCC mirror: g++.dg/contracts/cpp26/p3400-control-adl.C)

namespace label_ops {
  struct base_label_t {
    using assertion_control_object = base_label_t;
    int tag;
  };
  constexpr base_label_t alpha{0};
  constexpr base_label_t beta{1};

  struct combined_label_t {
    using assertion_control_object = combined_label_t;
    base_label_t left;
    base_label_t right;
  };

  constexpr combined_label_t operator|(base_label_t a, base_label_t b) {
    return combined_label_t{a, b};
  }
}

using contract_control namespace label_ops;

// operator| found via contract_control lookup in an assertion-control expr.
void f(int x) pre<(alpha | beta)>(x > 0) { }

constexpr auto combined = contract_control(alpha | beta);

void g(int x) pre<combined>(x > 0) { }

static_assert(__is_same(decltype(combined), const label_ops::combined_label_t));
