// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t && %t

// contract_control (E) in a template: a dependent operand is made into its
// constexpr object at instantiation, one object for each type and value, and
// it is an lvalue of its deduced type there as outside templates.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-control-template.C

#include <contracts>
#include <type_traits>

struct L { using assertion_control_object = L; int v; };
constexpr L make(int v) { return L{v}; }

template <int N> constexpr const L *addr() { return &contract_control(make(N)); }
template <class T> constexpr bool same() {
  return std::is_same_v<decltype((contract_control(T{}))), const T &>;
}

static_assert(addr<3>()->v == 3);
static_assert(addr<3>() == addr<3>());
static_assert(same<int>() && same<L>());

namespace lab {
constexpr L l{1};
}
template <class T> T f(T x) pre<contract_control(lab::l)>(x > 0) { return x; }

int main() { return f(1) - 1; }

// REQUIRES: contracts-libcxx, native
