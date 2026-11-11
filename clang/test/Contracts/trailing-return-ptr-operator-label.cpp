// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// A labelled contract after a trailing return type that ends in a
// ptr-operator is accepted: the abstract declarator stops at the contract
// keyword rather than taking `pre<obs>' for a template-id.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/trailing-return-ptr-operator-label.C

struct obs_t { using assertion_control_object = obs_t; };
constexpr obs_t obs{};
int g;

auto f(int x) -> int& pre<obs>(x > 0) { return g; }
auto h(int x) -> int* pre<obs>(x > 0) { return &g; }
struct S {
  auto m(int x) -> const int& pre<obs>(x > 0) { return g; }
};

// Controls, already accepted.
auto c1(int x) -> int pre<obs>(x > 0) { return g; }
auto c2(int x) -> int& pre(x > 0) { return g; }
