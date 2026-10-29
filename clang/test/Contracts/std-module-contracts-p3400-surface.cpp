// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe.pcm -fsyntax-only -Xclang -verify %t/use.cpp

// The std module's contracts partition (libcxx/modules/std/contracts.inc)
// exports every part of the P3400 library surface <contracts> declares: the
// evaluation_semantic_set members, the classification functions,
// allowed_semantics_t, the semantic restriction labels and the is_C variable
// templates, and D4405's analysis_semantic_label.  As in
// std-module-contracts-labels.cpp, the partition is exported from a small
// module rather than building all of std.
//
// Mirror: gcc/testsuite/g++.dg/modules/std-module-contract-p3400-surface.C
// in the gnu_gcc fork.

//--- probe.cppm
module;
#include <contracts>
export module probe;
#include "std/contracts.inc"

//--- use.cpp
// expected-no-diagnostics
import probe;

namespace C = std::contracts;
namespace L = std::contracts::labels;
using enum C::evaluation_semantic;

constexpr bool set_ops() {
  C::evaluation_semantic_set s;
  s.insert(observe);
  s.insert({enforce, ignore});
  s.erase(ignore);
  s |= C::evaluation_semantic_set{quick_enforce};
  s &= ~C::evaluation_semantic_set{quick_enforce};
  bool ok = s.size() == 2;
  s.clear();
  return ok && s.empty();
}
static_assert(set_ops());

static_assert(C::is_checked(observe) && C::is_unchecked(ignore));
static_assert(C::is_terminating(enforce) && C::is_continuing(observe));
static_assert(!C::is_terminating(assume) && !C::is_continuing(assume));

static_assert(L::allowed_semantics_t<observe>::allowed_semantics.contains(
    observe));
static_assert(L::terminating.allowed_semantics.contains(enforce));
static_assert(L::symbolic.allowed_semantics.contains(ignore));
static_assert(L::always_enforce.allowed_semantics.contains(enforce));
static_assert(
    L::always_quick_enforce.allowed_semantics.contains(quick_enforce));
static_assert(L::always_observe.allowed_semantics.contains(observe));
static_assert(L::always_ignore.allowed_semantics.contains(ignore));
static_assert(!L::never_enforce.allowed_semantics.contains(enforce));
static_assert(
    !L::never_quick_enforce.allowed_semantics.contains(quick_enforce));
static_assert(!L::never_observe.allowed_semantics.contains(observe));
static_assert(!L::never_ignore.allowed_semantics.contains(ignore));

static_assert(L::is_assertion_control_object<L::review>);
static_assert(L::is_semantic_computation_label<L::review>);
static_assert(L::is_allowed_semantics_label<L::terminating>);
static_assert(!L::is_local_violation_label<L::review>);
static_assert(!L::is_compute_comment_label<L::review>);
static_assert(!L::is_compute_message_label<L::review>);
static_assert(!L::is_queryable_label<L::review>);
static_assert(!L::is_identification_label<L::review>);
// D4405's analysis_semantic_label, with the labels terminating and symbolic
// now allowing prove.
static_assert(!L::analysis_semantic_label<decltype(L::terminating)>);
static_assert(L::terminating.allowed_semantics.contains(prove));
static_assert(L::symbolic.allowed_semantics.contains(prove));

// REQUIRES: contracts-libcxx
