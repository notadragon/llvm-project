// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore \
// RUN:   -fcontract-group-evaluation-semantic=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx:ignore \
// RUN:   -fcontract-group-evaluation-semantic=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxzz:ignore \
// RUN:   -fsyntax-only -verify %s

// P3400 [support.contract.control.ident]: a label's group names are read in
// full, however long, with every byte value, and however many there are (up
// to Clang's cap of 4096 names, p3400-group-names-too-many.cpp).  A name
// longer than 256 characters selects its own group and not the group of its
// first 256 characters; a name holding the bytes 0xFE or 0xFF is not the
// name without them, and a later name in the same range is still read; a
// newline does not split a name, so "x\nsafety" is not "safety"; and
// a range of 254, 255, 256 or 300 names is read to its last name.  Each
// configured group maps to ignore and everything else is enforced, so an
// assertion that selects the wrong group fails during constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3400-group-names-read-in-full.C (gnu_gcc).

// A name of 256 x followed by T1 T2 (both 0: just the 256 x).
struct long_name {
  char s[259];
  constexpr long_name(char t1, char t2) : s{} {
    for (int i = 0; i < 256; ++i)
      s[i] = 'x';
    s[256] = t1;
    s[257] = t2;
  }
};
constexpr long_name x256{0, 0}, x256y{'y', 0}, x256zz{'z', 'z'};

template <const char *N> struct one_name_t {
  using assertion_control_object = one_name_t;
  static constexpr const char *group_names[1] = {N};
};

// 256 x: configured, so ignored.
constexpr one_name_t<x256.s> l_x256{};
constexpr int f_x256(int x) {
  contract_assert<l_x256>(x > 0);
  return 1;
}
static_assert(f_x256(-1));

// 256 x then zz, 258 characters: configured on its own, so ignored.
constexpr one_name_t<x256zz.s> l_x256zz{};
constexpr int f_x256zz(int x) {
  contract_assert<l_x256zz>(x > 0);
  return 1;
}
static_assert(f_x256zz(-1));

// 256 x then y: not the 256-x group, and not configured, so enforced.
constexpr one_name_t<x256y.s> l_x256y{};
constexpr int f_x256y(int x) {
  contract_assert<l_x256y>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_x256y(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// Every byte value is part of a name: "safety\xfe" and "safety\xff" are
// not "safety", so enforced.
constexpr char safety_fe[] = "safety\xfe";
constexpr char safety_ff[] = "safety\xff";
constexpr char safety[] = "safety";

constexpr one_name_t<safety_fe> l_fe{};
constexpr int f_fe(int x) {
  contract_assert<l_fe>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_fe(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

constexpr one_name_t<safety_ff> l_ff{};
constexpr int f_ff(int x) {
  contract_assert<l_ff>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_ff(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// A 0xFF name does not end the range: "safety" after it selects the
// configured group, so ignored.
struct ff_then_safety_t {
  using assertion_control_object = ff_then_safety_t;
  static constexpr const char *group_names[2] = {safety_ff, safety};
};
constexpr ff_then_safety_t l_ff_safety{};
constexpr int f_ff_safety(int x) {
  contract_assert<l_ff_safety>(x > 0);
  return 1;
}
static_assert(f_ff_safety(-1));

// A newline inside a name does not split it: "x\nsafety" is not "safety",
// so enforced.
struct newline_t {
  using assertion_control_object = newline_t;
  static constexpr const char *group_names[1] = {"x\nsafety"};
};
constexpr newline_t l_newline{};
constexpr int f_newline(int x) {
  contract_assert<l_newline>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_newline(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// N names, only the last "safety": read in full, so ignored.
struct name_iterator {
  int i, last;
  constexpr const char *operator*() const { return i == last ? "safety" : "x"; }
  constexpr name_iterator &operator++() {
    ++i;
    return *this;
  }
  constexpr bool operator==(const name_iterator &) const = default;
};

template <int N> struct n_names {
  constexpr name_iterator begin() const { return {0, N - 1}; }
  constexpr name_iterator end() const { return {N, N - 1}; }
};

template <int N> struct n_t {
  using assertion_control_object = n_t;
  static constexpr n_names<N> group_names{};
};

template <int N> constexpr n_t<N> l_n{};

template <int N> constexpr int f_n(int x) {
  contract_assert<l_n<N>>(x > 0);
  return 1;
}
static_assert(f_n<254>(-1));
static_assert(f_n<255>(-1));
static_assert(f_n<256>(-1));
static_assert(f_n<300>(-1));

// Control: the same range with no "safety" is enforced.
struct no_safety_iterator {
  int i;
  constexpr const char *operator*() const { return "x"; }
  constexpr no_safety_iterator &operator++() {
    ++i;
    return *this;
  }
  constexpr bool operator==(const no_safety_iterator &) const = default;
};
struct no_safety_names {
  constexpr no_safety_iterator begin() const { return {0}; }
  constexpr no_safety_iterator end() const { return {255}; }
};
struct no_safety_t {
  using assertion_control_object = no_safety_t;
  static constexpr no_safety_names group_names{};
};
constexpr no_safety_t l_no_safety{};
constexpr int f_no_safety(int x) {
  contract_assert<l_no_safety>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_no_safety(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
