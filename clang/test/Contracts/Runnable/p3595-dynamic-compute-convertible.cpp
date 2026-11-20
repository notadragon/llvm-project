// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-compute-convertible.json %libcxx_flags -o %t && %t

// P3595 x P3400: on the dynamic path, as on the static one
// (../p3400-compute-semantic-convertible.cpp), a label is a computed-semantics
// control type only when its compute_semantic result converts implicitly to
// evaluation_semantic (CLANG-828).  The selector returns observe and every
// label maps it to ignore: the labels returning int, another enumeration and
// a class with only an explicit conversion do not participate, so their
// checks observe and call the handler; the label returning a class with an
// implicit conversion does, so its check is ignored.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-compute-convertible.C

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

struct int_t {
  using assertion_control_object = int_t;
  constexpr int compute_semantic(evaluation_semantic) const {
    return (int)evaluation_semantic::ignore;
  }
};
constexpr int_t int_l{};

enum class other : int { ignore_like = (int)evaluation_semantic::ignore };
struct enum_t {
  using assertion_control_object = enum_t;
  constexpr other compute_semantic(evaluation_semantic) const {
    return other::ignore_like;
  }
};
constexpr enum_t enum_l{};

struct explicit_sem {
  constexpr explicit operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct explicit_t {
  using assertion_control_object = explicit_t;
  constexpr explicit_sem compute_semantic(evaluation_semantic) const {
    return {};
  }
};
constexpr explicit_t explicit_l{};

struct implicit_sem {
  constexpr operator evaluation_semantic() const {
    return evaluation_semantic::ignore;
  }
};
struct implicit_t {
  using assertion_control_object = implicit_t;
  constexpr implicit_sem compute_semantic(evaluation_semantic) const {
    return {};
  }
};
constexpr implicit_t implicit_l{};

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

evaluation_semantic p3595_sel_compute() { return evaluation_semantic::observe; }

void f_int(int x) pre<int_l>(x > 0) {}
void f_enum(int x) pre<enum_l>(x > 0) {}
void f_explicit(int x) pre<explicit_l>(x > 0) {}
void f_implicit(int x) pre<implicit_l>(x > 0) {}

int main() {
  f_int(-1); // observe: handler
  if (violations != 1)
    std::abort();
  f_enum(-1); // observe: handler
  if (violations != 2)
    std::abort();
  f_explicit(-1); // observe: handler
  if (violations != 3)
    std::abort();
  f_implicit(-1); // ignore: no handler
  if (violations != 3)
    std::abort();
}
