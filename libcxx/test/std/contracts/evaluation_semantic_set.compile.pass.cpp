//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: contracts

// ADDITIONAL_COMPILE_FLAGS: -fcontracts

// <contracts>
//
// P3400 [support.contract.control.semset]: every member of
// evaluation_semantic_set, in constant evaluation.  all(), ~ and the never_X
// labels cover every semantic this implementation provides, including the
// extensions assume, noexcept_observe, noexcept_enforce, prove and diagnose,
// but not unspecified, which is not a semantic (notadragon_wg21
// DECISIONS-resolved.md E65).
//
// Mirror: libstdc++-v3/testsuite/18_support/contracts/evaluation_semantic_set.cc
// in the gnu_gcc fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

constexpr evaluation_semantic every[] = {
    ignore, observe, enforce, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose};

static_assert(evaluation_semantic_set().empty());
static_assert(evaluation_semantic_set().size() == 0);
static_assert(evaluation_semantic_set::none() == evaluation_semantic_set());

constexpr bool check_all() {
  evaluation_semantic_set a = evaluation_semantic_set::all();
  for (auto s : every)
    if (!a.contains(s))
      return false;
  return !a.contains(unspecified) && a.size() == 9;
}
static_assert(check_all());

static_assert(evaluation_semantic_set{observe, enforce}.size() == 2);
static_assert(evaluation_semantic_set{observe, observe}.size() == 1);

constexpr bool check_modifiers() {
  evaluation_semantic_set s;
  s.insert(observe);
  if (!s.contains(observe) || s.size() != 1)
    return false;
  s.insert({enforce, quick_enforce});
  if (s != evaluation_semantic_set{observe, enforce, quick_enforce})
    return false;
  s.erase(enforce);
  s.erase(ignore); // not an element: no effect
  if (s != evaluation_semantic_set{observe, quick_enforce})
    return false;
  s.clear();
  return s.empty();
}
static_assert(check_modifiers());

constexpr bool check_compound() {
  evaluation_semantic_set s{observe, enforce};
  evaluation_semantic_set& r = (s &= evaluation_semantic_set{enforce, ignore});
  if (&r != &s || s != evaluation_semantic_set{enforce})
    return false;
  evaluation_semantic_set& q = (s |= evaluation_semantic_set{ignore});
  return &q == &s && s == evaluation_semantic_set{enforce, ignore};
}
static_assert(check_compound());

constexpr bool check_assign() {
  evaluation_semantic_set s{observe};
  evaluation_semantic_set t;
  evaluation_semantic_set& r = (t = s);
  return &r == &t && t == s;
}
static_assert(check_assign());

static_assert((evaluation_semantic_set{observe, enforce} & evaluation_semantic_set{enforce, ignore}) ==
              evaluation_semantic_set{enforce});
static_assert((evaluation_semantic_set{observe} | evaluation_semantic_set{ignore}) ==
              evaluation_semantic_set{observe, ignore});

// ~ complements over all().
static_assert(~evaluation_semantic_set() == evaluation_semantic_set::all());
static_assert(~evaluation_semantic_set::all() == evaluation_semantic_set());
static_assert(~evaluation_semantic_set{ignore, observe} ==
              evaluation_semantic_set{
                  enforce, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose});
static_assert(!(~evaluation_semantic_set{ignore}).contains(unspecified));
static_assert(!(~evaluation_semantic_set{unspecified}).contains(unspecified));
