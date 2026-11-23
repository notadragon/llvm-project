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
// P4405: the prove and diagnose enumerators of evaluation_semantic (8 and 9,
// the next values after the P4298 extensions), their place in
// evaluation_semantic_set's all() and ~, and their classification: like
// ignore, neither is checked nor terminating, both are continuing, and
// neither lets an exception out of handling.  The enumerators are
// unconditional, as assume's is.
//
// Mirror: libstdc++-v3/testsuite/18_support/contracts/p4405_semantics.cc in
// the gnu_gcc fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::is_checked;
using std::contracts::is_continuing;
using std::contracts::is_nonthrowing;
using std::contracts::is_terminating;
using std::contracts::is_unchecked;
using enum evaluation_semantic;

static_assert(static_cast<int>(prove) == 8);
static_assert(static_cast<int>(diagnose) == 9);

static_assert(evaluation_semantic_set::all().contains(prove));
static_assert(evaluation_semantic_set::all().contains(diagnose));
static_assert((~evaluation_semantic_set{prove}) == (evaluation_semantic_set::all() & ~evaluation_semantic_set{prove}));
static_assert(!(~evaluation_semantic_set{prove}).contains(prove));
static_assert((~evaluation_semantic_set{prove}).contains(diagnose));
static_assert((~evaluation_semantic_set::all()).empty());
static_assert(evaluation_semantic_set{prove, diagnose}.size() == 2);

static_assert(!is_checked(prove));
static_assert(!is_checked(diagnose));
static_assert(is_unchecked(prove));
static_assert(is_unchecked(diagnose));
static_assert(!is_terminating(prove));
static_assert(!is_terminating(diagnose));
static_assert(is_continuing(prove));
static_assert(is_continuing(diagnose));
static_assert(is_nonthrowing(prove));
static_assert(is_nonthrowing(diagnose));
