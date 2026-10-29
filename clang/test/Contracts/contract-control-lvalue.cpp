// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// An assertion-control-expression "is an lvalue denoting a constexpr object
// with a deduced type initialized by its constant-expression" (P3400,
// [expr.contract.control]).  The non-constant-operand half is
// contract-control-nonconstant.cpp, the postfix placement
// contract-control-postfix.cpp, templates contract-control-template.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-control-lvalue.C

#include <type_traits>
struct L { using assertion_control_object = L; int v; };
constexpr L make() { return L{3}; }

static_assert(std::is_same_v<decltype((contract_control(42))), const int &>);
static_assert(std::is_same_v<decltype((contract_control(make()))), const L &>);
constexpr const L *p = &contract_control(make());

// REQUIRES: contracts-libcxx
