// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-inline-namespace.json %libcxx_flags -o %t && %t

// P3595: a "C++"-linkage selector name is looked up as a qualified name, so
// "lib::my_sel" finds a selector declared in an inline namespace lib::v1,
// and "lib::tagged" one declared with an abi_tag, and the check calls that
// function rather than a synthesized lib::my_sel() / lib::tagged().  With
// "provideweak", calling the synthesized weak default (enforce) would
// abort; the user's selectors say observe, so both violations continue.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-inline-namespace.C

#include <contracts>

namespace lib {
inline namespace v1 {
std::contracts::evaluation_semantic my_sel() {
  return std::contracts::evaluation_semantic::observe;
}
} // namespace v1
[[gnu::abi_tag("x")]] std::contracts::evaluation_semantic tagged() {
  return std::contracts::evaluation_semantic::observe;
}
} // namespace lib

int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

namespace n1 {
void f(int x) pre(x > 0) {}
} // namespace n1
namespace n2 {
void f(int x) pre(x > 0) {}
} // namespace n2

int main() {
  n1::f(-1);
  n2::f(-1);
  if (violations != 2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
