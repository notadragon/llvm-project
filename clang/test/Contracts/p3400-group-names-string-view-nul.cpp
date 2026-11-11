// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore,lib.part:ignore,ub:conv.lval.valid.representation.bool.enum:ignore \
// RUN:   -fsyntax-only \
// RUN:   -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E191 (a), E192 (2): a group name read as a std::string_view is the whole
// view, NUL bytes included, and the configuration matches it on all of its
// bytes.  No configured name holds a NUL, so "safety\0x" and "safety\0" are
// not the safety group; a name after one holding a NUL is still read; and a
// name holding a NUL is within a configured group that it starts with
// followed by a dot, as any name is: "safety.x\0y" is within safety,
// "safety\0.x" is not.  A const char* name still ends at its NUL, so
// "safety\0x" through a const char* is "safety".
//
// A configured group that is a prefix of a name up to its NUL selects it
// only through a dot: with lib.part configured, "lib.part\0x",
// "lib.part\0" and "lib.part\0.x" are not selected, "lib.part.x\0y" is,
// and "lib\0.part" and "lib.partial\0" are not.  The combined P3100 group
// ub:conv.lval.valid.representation.bool.enum selects names within its
// leaves the same way.
//
// The configured groups are ignored and everything else is enforced, so an
// assertion that selects the wrong group fails during constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3400-group-names-string-view-nul.C (gnu_gcc).

#include <string_view>

using namespace std::string_view_literals;

template <const std::string_view &N> struct one_t {
  using assertion_control_object = one_t;
  static constexpr std::string_view group_names[1] = {N};
};

// A NUL byte inside a name: "safety\0x" is not "safety", so enforced.
constexpr std::string_view inner = "safety\0x"sv;
constexpr one_t<inner> inner_label{};
constexpr int f_inner(int x) {
  contract_assert<inner_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_inner(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// A NUL byte ending a name: "safety\0" is not "safety" either.
constexpr std::string_view ending = "safety\0"sv;
constexpr one_t<ending> end_label{};
constexpr int f_end(int x) {
  contract_assert<end_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_end(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// A name within safety, holding a NUL after the dot: ignored.
constexpr std::string_view within = "safety.x\0y"sv;
constexpr one_t<within> within_label{};
constexpr int f_within(int x) {
  contract_assert<within_label>(x > 0);
  return 1;
}
static_assert(f_within(-1));

// The NUL before the dot: "safety\0.x" is not within safety, so enforced.
constexpr std::string_view before_dot = "safety\0.x"sv;
constexpr one_t<before_dot> before_dot_label{};
constexpr int f_before_dot(int x) {
  contract_assert<before_dot_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_before_dot(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// After a name holding a NUL byte, "safety" is still read: ignored.
struct then_t {
  using assertion_control_object = then_t;
  static constexpr std::string_view group_names[2] = {"safety\0x"sv,
                                                      "safety"sv};
};
constexpr then_t then_label{};
constexpr int f_then(int x) {
  contract_assert<then_label>(x > 0);
  return 1;
}
static_assert(f_then(-1));

// A const char* name ends at its NUL: "safety\0x" is "safety", so ignored.
struct pointer_t {
  using assertion_control_object = pointer_t;
  static constexpr const char *group_names[1] = {"safety\0x"};
};
constexpr pointer_t pointer_label{};
constexpr int f_pointer(int x) {
  contract_assert<pointer_label>(x > 0);
  return 1;
}
static_assert(f_pointer(-1));

// The configured lib.part is a prefix of "lib.part\0x" up to the NUL, but
// that is not a dot: enforced.
constexpr std::string_view part_inner = "lib.part\0x"sv;
constexpr one_t<part_inner> part_inner_label{};
constexpr int f_part_inner(int x) {
  contract_assert<part_inner_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_part_inner(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// "lib.part\0" is not lib.part: enforced.
constexpr std::string_view part_end = "lib.part\0"sv;
constexpr one_t<part_end> part_end_label{};
constexpr int f_part_end(int x) {
  contract_assert<part_end_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_part_end(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// "lib.part\0.x" is not within lib.part: enforced.
constexpr std::string_view part_before_dot = "lib.part\0.x"sv;
constexpr one_t<part_before_dot> part_before_dot_label{};
constexpr int f_part_before_dot(int x) {
  contract_assert<part_before_dot_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_part_before_dot(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// "lib.part.x\0y" is within lib.part: ignored.
constexpr std::string_view part_within = "lib.part.x\0y"sv;
constexpr one_t<part_within> part_within_label{};
constexpr int f_part_within(int x) {
  contract_assert<part_within_label>(x > 0);
  return 1;
}
static_assert(f_part_within(-1));

// "lib\0.part" holds the configured name's bytes only around the NUL:
// enforced.
constexpr std::string_view part_split = "lib\0.part"sv;
constexpr one_t<part_split> part_split_label{};
constexpr int f_part_split(int x) {
  contract_assert<part_split_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_part_split(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// "lib.partial\0" starts with lib.part, but not followed by a dot:
// enforced.
constexpr std::string_view partial = "lib.partial\0"sv;
constexpr one_t<partial> partial_label{};
constexpr int f_partial(int x) {
  contract_assert<partial_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_partial(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// The combined group selects within its .bool leaf, but
// "ub:conv.lval.valid.representation.bool\0x" is not within it: enforced.
constexpr std::string_view leaf_inner =
    "ub:conv.lval.valid.representation.bool\0x"sv;
constexpr one_t<leaf_inner> leaf_inner_label{};
constexpr int f_leaf_inner(int x) {
  contract_assert<leaf_inner_label>(x > 0); // expected-error {{contract failed during execution of constexpr function}}
  return 1;
}
static_assert(f_leaf_inner(-1)); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}

// "ub:conv.lval.valid.representation.enum.x\0y" is within the .enum leaf:
// ignored.
constexpr std::string_view leaf_within =
    "ub:conv.lval.valid.representation.enum.x\0y"sv;
constexpr one_t<leaf_within> leaf_within_label{};
constexpr int f_leaf_within(int x) {
  contract_assert<leaf_within_label>(x > 0);
  return 1;
}
static_assert(f_leaf_within(-1));
