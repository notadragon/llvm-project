// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=quick_enforce \
// RUN:   -fcontract-group-evaluation-semantic=safety:ignore \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// REQUIRES: contracts-libcxx, native

// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E191 (a), E192 (2), at run time: a group name read as a std::string_view
// is the whole view, NUL bytes included, and selects the semantic of a
// run-time check on all of its bytes.  With the safety group ignored and
// everything else quick_enforce, "safety\0x", "safety\0" and "safety\0.x"
// are checked; "safety.x\0y", a name after one holding a NUL, and a
// const char* name "safety\0x" (which ends at its NUL) are in the ignored
// safety group.  Each checked assertion prints its name; an ignored one
// does not evaluate its predicate.  The constant-evaluation half is
// ../p3400-group-names-string-view-nul.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3400-group-names-string-view-nul-run.C (gnu_gcc), and
// tests/tests/contracts/p3400groups/run_string_view.sft.cpp
// (edgcpp_compiler).

#include <cstdio>
#include <string_view>

using namespace std::string_view_literals;

static bool checked(const char *name) {
  std::printf("%s checked\n", name);
  return true;
}

template <const std::string_view &N> struct one_t {
  using assertion_control_object = one_t;
  static constexpr std::string_view group_names[1] = {N};
};

constexpr std::string_view inner = "safety\0x"sv;
constexpr one_t<inner> inner_label{};
constexpr std::string_view ending = "safety\0"sv;
constexpr one_t<ending> end_label{};
constexpr std::string_view within = "safety.x\0y"sv;
constexpr one_t<within> within_label{};
constexpr std::string_view before_dot = "safety\0.x"sv;
constexpr one_t<before_dot> before_dot_label{};

struct then_t {
  using assertion_control_object = then_t;
  static constexpr std::string_view group_names[2] = {"safety\0x"sv,
                                                      "safety"sv};
};
constexpr then_t then_label{};

struct pointer_t {
  using assertion_control_object = pointer_t;
  static constexpr const char *group_names[1] = {"safety\0x"};
};
constexpr pointer_t pointer_label{};

int main() {
  contract_assert<inner_label>(checked("inner"));
  contract_assert<end_label>(checked("end"));
  contract_assert<within_label>(checked("within"));
  contract_assert<before_dot_label>(checked("before_dot"));
  contract_assert<then_label>(checked("then"));
  contract_assert<pointer_label>(checked("pointer"));
  std::puts("done");
  return 0;
}

// CHECK: {{^}}inner checked
// CHECK-NEXT: {{^}}end checked
// CHECK-NEXT: {{^}}before_dot checked
// CHECK-NEXT: {{^}}done
// CHECK-NOT: checked
