// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fsyntax-only %s
// REQUIRES: contracts-libcxx

// P3400 [support.contract.control.semset.mod] names the member that takes a
// semantic out of an evaluation_semantic_set erase(evaluation_semantic), as
// std::set does (CLANG-827).  The earlier name, remove, is not kept as an
// alias.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-semantic-set-erase.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template <typename Set>
concept has_erase = requires(Set s) { s.erase(evaluation_semantic::observe); };

template <typename Set>
concept has_remove =
    requires(Set s) { s.remove(evaluation_semantic::observe); };

// Control: the same form of check finds a member that exists.
template <typename Set>
concept has_insert =
    requires(Set s) { s.insert(evaluation_semantic::observe); };

static_assert(has_insert<evaluation_semantic_set>);
static_assert(has_erase<evaluation_semantic_set>);
static_assert(!has_remove<evaluation_semantic_set>);

consteval bool erase_works() {
  evaluation_semantic_set s{evaluation_semantic::ignore,
                            evaluation_semantic::observe};
  s.erase(evaluation_semantic::observe);
  // Erasing a semantic the set does not contain changes nothing.
  s.erase(evaluation_semantic::enforce);
  return !s.contains(evaluation_semantic::observe) &&
         s.contains(evaluation_semantic::ignore) && s.size() == 1;
}
static_assert(erase_works());
