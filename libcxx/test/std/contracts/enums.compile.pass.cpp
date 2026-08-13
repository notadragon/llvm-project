//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: contracts

// ADDITIONAL_COMPILE_FLAGS: -fcontracts -fcontracts-p3290

// <contracts>
//
// The enumerator values are ABI, not an implementation detail: the compiler
// emits them into the violation object and libstdc++ must agree with them, so
// pin the numbers rather than merely checking the names exist.

#include <contracts>
#include <type_traits>

using namespace std::contracts;

// [support.contract.enum]
static_assert(static_cast<int>(assertion_kind::pre) == 1);
static_assert(static_cast<int>(assertion_kind::post) == 2);
static_assert(static_cast<int>(assertion_kind::assert) == 3);
static_assert(static_cast<int>(assertion_kind::manual) == 4);
static_assert(static_cast<int>(assertion_kind::cassert) == 5);

static_assert(static_cast<int>(evaluation_semantic::ignore) == 1);
static_assert(static_cast<int>(evaluation_semantic::observe) == 2);
static_assert(static_cast<int>(evaluation_semantic::enforce) == 3);
static_assert(static_cast<int>(evaluation_semantic::quick_enforce) == 4);

static_assert(static_cast<int>(detection_mode::predicate_false) == 1);
static_assert(static_cast<int>(detection_mode::evaluation_exception) == 2);

// The underlying type of evaluation_semantic is 16-bit to match libstdc++,
// which spells it __UINT16_TYPE__.  Getting this wrong is silently ABI-breaking
// across the two standard libraries.
static_assert(sizeof(std::underlying_type_t<evaluation_semantic>) == 2);
static_assert(sizeof(std::underlying_type_t<assertion_kind>) == 1);
static_assert(sizeof(std::underlying_type_t<detection_mode>) == 1);

// evaluation_semantic_set is pure library, so it is fully constexpr-testable.
constexpr evaluation_semantic_set empty_set{};
static_assert(empty_set.empty());
static_assert(!empty_set.contains(evaluation_semantic::enforce));

constexpr evaluation_semantic_set one{evaluation_semantic::observe};
static_assert(!one.empty());
static_assert(one.contains(evaluation_semantic::observe));
static_assert(!one.contains(evaluation_semantic::enforce));

constexpr evaluation_semantic_set two{evaluation_semantic::observe,
                                      evaluation_semantic::enforce};
static_assert(two.contains(evaluation_semantic::observe));
static_assert(two.contains(evaluation_semantic::enforce));
static_assert(!two.contains(evaluation_semantic::ignore));
