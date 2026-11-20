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
// P3400 [support.contract.control.semfun]: is_checked, is_unchecked,
// is_terminating, is_continuing.  The extensions are classified by behaviour
// (notadragon_wg21 DECISIONS-resolved.md E65): noexcept_observe and
// noexcept_enforce are checked, noexcept_enforce is terminating, and assume,
// prove, diagnose and unspecified are unchecked.  prove, diagnose and
// unspecified are continuing (unspecified per DECISIONS-resolved.md E142);
// assume is neither terminating nor continuing, since continuing after a
// violation under it is undefined behaviour (D3400R5; CLANG-825).
//
// Mirror: libstdc++-v3/testsuite/18_support/contracts/classification.cc in
// the gnu_gcc fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::is_checked;
using std::contracts::is_continuing;
using std::contracts::is_terminating;
using std::contracts::is_unchecked;
using enum evaluation_semantic;

static_assert(is_checked(observe));
static_assert(is_checked(enforce));
static_assert(is_checked(quick_enforce));
static_assert(is_checked(noexcept_observe));
static_assert(is_checked(noexcept_enforce));
static_assert(!is_checked(ignore));
static_assert(!is_checked(assume));
static_assert(!is_checked(prove));
static_assert(!is_checked(diagnose));
static_assert(!is_checked(unspecified));

static_assert(is_terminating(enforce));
static_assert(is_terminating(quick_enforce));
static_assert(is_terminating(noexcept_enforce));
static_assert(!is_terminating(observe));
static_assert(!is_terminating(noexcept_observe));
static_assert(!is_terminating(ignore));
static_assert(!is_terminating(assume));
static_assert(!is_terminating(prove));
static_assert(!is_terminating(diagnose));
static_assert(!is_terminating(unspecified));

static_assert(is_continuing(ignore));
static_assert(is_continuing(observe));
static_assert(is_continuing(noexcept_observe));
static_assert(is_continuing(prove));
static_assert(is_continuing(diagnose));
static_assert(is_continuing(unspecified));
static_assert(!is_continuing(enforce));
static_assert(!is_continuing(quick_enforce));
static_assert(!is_continuing(noexcept_enforce));
static_assert(!is_continuing(assume));

constexpr bool check_negations() {
  for (auto s :
       {unspecified, ignore, observe, enforce, quick_enforce, assume, noexcept_observe, noexcept_enforce, prove, diagnose})
    if (is_unchecked(s) == is_checked(s) || (s != assume && is_continuing(s) == is_terminating(s)))
      return false;
  return true;
}
static_assert(check_negations());
