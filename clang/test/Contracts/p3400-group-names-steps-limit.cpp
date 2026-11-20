// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore \
// RUN:   -fconstexpr-steps=1000 -fsyntax-only -verify %s

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E189 (a): a label whose group_names is a range too long to read within
// the constexpr step limit is an error, with the evaluator's note of the
// limit, rather than a label silently in no group.  No header: the names
// are read as const char*, and nothing else here comes near the lowered
// limit.  The range is walked once per reading (the count, the lengths,
// the characters), so the limit bounds one such walk; the cap on the
// number of names is p3400-group-names-too-many.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-ops-limit.C

struct name_iterator {
  int i;
  constexpr const char *operator*() const { return "safety"; }
  constexpr name_iterator &operator++() { ++i; return *this; }
  constexpr bool operator==(const name_iterator &) const = default;
};

struct many_names {
  constexpr name_iterator begin() const { return {0}; }
  constexpr name_iterator end() const { return {100000}; }
};

struct many_t {
  using assertion_control_object = many_t;
  static constexpr many_names group_names{};
};
constexpr many_t many{};

struct few_names {
  constexpr name_iterator begin() const { return {0}; }
  constexpr name_iterator end() const { return {1}; }
};

struct few_t {
  using assertion_control_object = few_t;
  static constexpr few_names group_names{};
};
constexpr few_t few{};

void f(int x) pre<many>(x > 0) {}  // expected-error {{group_names facet of assertion-control object 'many_t' must be usable in a constant expression}}
void g(int x) pre<few>(x > 0) {}

// The evaluator's notes: the limit, where in the reading it was hit, and
// the option that raises it.
// expected-note@*:* {{constexpr evaluation hit maximum step limit of 1000}}
// expected-note@*:* 0+ {{in call to}}
// expected-note@*:* {{use -fconstexpr-steps= to increase this limit}}
