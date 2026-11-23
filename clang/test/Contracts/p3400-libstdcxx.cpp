// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ -fsyntax-only \
// RUN:   -fcontracts-p3400 -Xclang -verify %s
// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ -fsyntax-only \
// RUN:   -fcontracts-p3400 -fcontracts-p4405 -Xclang -verify %s
// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ -fsyntax-only \
// RUN:   -fcontracts-p3400 -fcontracts-p4451 -Xclang -verify %s
// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ \
// RUN:   -fcontracts-p3400 -fcontract-evaluation-semantic=ignore \
// RUN:   -S -emit-llvm -o - %s | FileCheck %s
// REQUIRES: contracts-libstdcxx
// expected-no-diagnostics

// P3400 with libstdc++: libstdc++'s <contracts> provides the P3400 library
// (keyed on __cpp_contracts_labels) to this compiler too.  Group labels
// spelled "name"group outside an assertion (through a using-declaration) and
// inside one (where the label's operators are found without it); combined
// groups, whose names are the sorted, duplicate-free union of the operands'
// whether they are a group's char[1][N] or a user label's array of
// pointers; and the library's control objects.
//
// Mirror: libstdc++-v3/include/std/contracts

#include <contracts>
#include <string_view>

#if !(__cpp_contracts_labels > 0)
#error "__cpp_contracts_labels"
#endif

namespace ct = std::contracts;
namespace scl = std::contracts::controls;
using scl::operator|;
using scl::operator""group;
using ES = ct::evaluation_semantic;

// Group names visited in order, joined by ','.
constexpr bool names_are(const auto &L, std::string_view want) {
  std::size_t pos = 0;
  bool first = true;
  for (std::string_view n : L.group_names) {
    if (!first) {
      if (pos >= want.size() || want[pos] != ',')
        return false;
      ++pos;
    }
    first = false;
    if (want.substr(pos, n.size()) != n)
      return false;
    pos += n.size();
  }
  return pos == want.size();
}

// A user label whose group_names is an array of pointers.
struct ptr_groups_t {
  using assertion_control_object = ptr_groups_t;
  static constexpr const char *group_names[2] = {"zeta", "beta"};
};
constexpr ptr_groups_t ptr_groups{};

constexpr auto g_one = "safety"group;
constexpr auto g_two = "zeta"group | "alpha"group;   // char[1][N] | char[1][N]
constexpr auto g_three = g_two | "beta"group;
constexpr auto g_ptr = ptr_groups | "alpha"group;    // const char *[2] | group
constexpr auto g_nest =
    ("safety"group | ptr_groups) | ("audit"group | "safety"group);
static_assert(scl::is_identification_label<g_one>);
static_assert(scl::is_identification_label<g_two>);
// A combination holding a user label's pointers to string literals is not
// a valid template argument, so ask the concept of its type.
static_assert(scl::identification_label<decltype(g_ptr)>);
static_assert(scl::identification_label<decltype(g_nest)>);
static_assert(names_are(g_one, "safety"));
static_assert(names_are(g_two, "alpha,zeta"));
static_assert(names_are(g_three, "alpha,beta,zeta"));
static_assert(names_are(g_ptr, "alpha,beta,zeta"));
static_assert(names_are(g_nest, "audit,beta,safety,zeta"));

// controls::enforced allows enforce and not ignore.
static_assert(scl::is_allowed_semantics_label<scl::enforced>);
static_assert(ct::evaluation_semantic_set(scl::enforced.allowed_semantics)
                  .contains(ES::enforce));
static_assert(!ct::evaluation_semantic_set(scl::enforced.allowed_semantics)
                   .contains(ES::ignore));
static_assert(scl::is_semantic_computation_label<scl::review>);

// CHECK-LABEL: define {{.*}}@_Z8f_safetyi(
// CHECK-NOT: call void @__cxa_contract_violation
// CHECK: ret void
void f_safety(int x) pre<"safety"group>(x > 0) {}
void f_both(int x) pre<("audit"group | "safety"group)>(x > 0) {}
void f_nest(int x) pre<g_nest>(x > 0) {}
void f_ptr(int x) pre<(ptr_groups | "alpha"group)>(x > 0) {}
void f_always_ignore(int x) pre<scl::always_ignore>(x > 0) {}
void f_always_observe(int x) pre<scl::always_observe>(x > 0) {}
void f_never_enforce(int x) pre<scl::never_enforce>(x > 0) {}
void f_review(int x) pre<scl::review>(x > 0) {}
// CHECK-LABEL: define {{.*}}@_Z10f_enforcedi(
// CHECK: call void @__cxa_contract_violation_pre_enforce
void f_enforced(int x) pre<scl::enforced>(x > 0) {}
void g_enforced(int x) { contract_assert<scl::enforced>(x > 0); }
void f_msg(int x)
    pre<(scl::fixed_message_label_t("m") | "safety"group)>(x > 0) {}
#ifdef __cpp_contracts_prove
void f_prove(int x) pre<scl::prove>(x > 0) {}
void f_always_prove(int x) pre<scl::always_prove>(x > 0) {}
#endif
