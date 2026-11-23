// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore -fsyntax-only \
// RUN:   -verify %s

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E120, E189 (a), E190: a label whose group_names cannot be read -- a name
// that is not a constant, more than 4096 names -- is diagnosed once, at the
// label, also when the assertion is constant-evaluated, possibly more than
// once.  -verify matches each diagnostic exactly once, so a second report
// would be an unexpected diagnostic.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3400-group-names-reported-once.C (gnu_gcc).

extern const char *const rn[1]; // expected-note 3 {{declared here}}
struct rr {
  constexpr const char *const *begin() const { return rn; }
  constexpr const char *const *end() const { return rn + 1; }
};
struct lab {
  using assertion_control_object = lab;
  static constexpr rr group_names{};
};
constexpr lab l{};

// 4097 names: one more than a label may hold.
struct many_t {
  using assertion_control_object = many_t;
  static constexpr const char *group_names[4097] = {"a"};
};
constexpr many_t many{};

// An assertion statement, constant-evaluated twice.
constexpr int f(int x) {
  contract_assert<l>(x > 0); // expected-error {{group_names facet of assertion-control object 'lab' must be usable in a constant expression}} \
                             // expected-note {{read of non-constexpr variable 'rn' is not allowed in a constant expression}}
  return 1;
}
static_assert(f(1));
static_assert(f(2));

// A precondition.
constexpr int g(int x) pre<l>(x > 0) { // expected-error {{group_names facet of assertion-control object 'lab' must be usable in a constant expression}} \
                                       // expected-note {{read of non-constexpr variable 'rn' is not allowed in a constant expression}}
  return 1;
}
static_assert(g(1));

// A prvalue label.
constexpr int h(int x) {
  contract_assert<lab{}>(x > 0); // expected-error {{group_names facet of assertion-control object 'lab' must be usable in a constant expression}} \
                                 // expected-note {{read of non-constexpr variable 'rn' is not allowed in a constant expression}}
  return 1;
}
static_assert(h(1));

// Too many names.
constexpr int k(int x) {
  contract_assert<many>(x > 0); // expected-error {{group_names facet of assertion-control object 'many_t' holds more than 4096 names}}
  return 1;
}
static_assert(k(1));
