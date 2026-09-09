// D4298 + P3400: is_nonthrowing classifies the semantics that never
// propagate an exception out of contract-violation handling.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-p3400-is-nonthrowing.C)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 %libcxx_flags -o %t
// RUN: %t

#include <contracts>
using namespace std::contracts;

// is_nonthrowing is constexpr, so classification also holds at compile time.
static_assert(is_nonthrowing(evaluation_semantic::ignore));
static_assert(is_nonthrowing(evaluation_semantic::quick_enforce));
static_assert(is_nonthrowing(evaluation_semantic::assume));
static_assert(is_nonthrowing(evaluation_semantic::noexcept_enforce));
static_assert(is_nonthrowing(evaluation_semantic::noexcept_observe));
static_assert(!is_nonthrowing(evaluation_semantic::enforce));
static_assert(!is_nonthrowing(evaluation_semantic::observe));

int main()
{
  if (!is_nonthrowing(evaluation_semantic::ignore)) __builtin_trap();
  if (!is_nonthrowing(evaluation_semantic::quick_enforce)) __builtin_trap();
  if (!is_nonthrowing(evaluation_semantic::assume)) __builtin_trap();
  if (!is_nonthrowing(evaluation_semantic::noexcept_enforce)) __builtin_trap();
  if (!is_nonthrowing(evaluation_semantic::noexcept_observe)) __builtin_trap();
  if (is_nonthrowing(evaluation_semantic::enforce)) __builtin_trap();
  if (is_nonthrowing(evaluation_semantic::observe)) __builtin_trap();
}
