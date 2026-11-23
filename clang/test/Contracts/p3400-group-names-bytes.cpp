// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-configuration-file=%S/p3400-group-names-bytes.json \
// RUN:   -fsyntax-only \
// RUN:   -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400 and notadragon_wg21 DECISIONS.md E192 (2): a label group name is
// matched on all of its bytes, whichever they are.  With the groups "n\nx",
// "s\001x" and "z0" configured (p3400-group-names-bytes.json), the label
// groups "n\nx", "s\001x" and "z0" are selected and "n\001x", "s\nx" and
// "z\0" are not: the bytes 0, newline and 1 are kept apart from each other
// and from the other bytes through matching.  (A JSON group may hold a
// newline or the byte 1, not a NUL.)
//
// The configured groups are ignored and everything else is enforced, so an
// assertion that selects the wrong group fails during constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-bytes.C
// (gnu_gcc); tests/tests/contracts/p3400groups/name_bytes.sft.cpp
// (edgcpp_compiler).

#include <string_view>

using namespace std::string_view_literals;

template <const std::string_view &N> struct one_t {
  using assertion_control_object = one_t;
  static constexpr std::string_view group_names[1] = {N};
};

// Selected: ignored.
constexpr std::string_view nl = "n\nx"sv;
constexpr one_t<nl> nl_label{};
constexpr std::string_view one = "s\001x"sv;
constexpr one_t<one> one_label{};
constexpr std::string_view zero = "z0"sv;
constexpr one_t<zero> zero_label{};

// Not selected: enforced.
constexpr std::string_view nl_one = "n\001x"sv;
constexpr one_t<nl_one> nl_one_label{};
constexpr std::string_view one_nl = "s\nx"sv;
constexpr one_t<one_nl> one_nl_label{};
constexpr std::string_view nul = "z\0"sv;
constexpr one_t<nul> nul_label{};

constexpr int f_nl(int x) {
  contract_assert<nl_label>(x > 0);
  return 1;
}
static_assert(f_nl(-1));

constexpr int f_one(int x) {
  contract_assert<one_label>(x > 0);
  return 1;
}
static_assert(f_one(-1));

constexpr int f_zero(int x) {
  contract_assert<zero_label>(x > 0);
  return 1;
}
static_assert(f_zero(-1));

constexpr int f_nl_one(int x) {
  contract_assert<nl_one_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_nl_one(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

constexpr int f_one_nl(int x) {
  contract_assert<one_nl_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_one_nl(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

constexpr int f_nul(int x) {
  contract_assert<nul_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_nul(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
