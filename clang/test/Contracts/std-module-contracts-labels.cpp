// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe.pcm
// RUN: %clangxx -std=c++26 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe.pcm -fsyntax-only -Xclang -verify %t/use.cpp
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe-p3850.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe-p3850.pcm -fsyntax-only -Xclang -verify %t/use.cpp

// The std module's contracts partition (libcxx/modules/std/contracts.inc)
// exports std::contracts::labels, including the concept
// assertion_control_object, whether or not the compiler implements P3400
// labels, as <contracts> declares them (CLANG-667, found by libc++'s
// module_std.gen.py).  As in std-module-contracts-names.cpp, the partition is
// exported from a small module rather than building all of std.
//
// Mirror: gcc/testsuite/g++.dg/modules/std-module-contract-labels.C

//--- probe.cppm
module;
#include <contracts>
export module probe;
#include "std/contracts.inc"

//--- use.cpp
// expected-no-diagnostics
import probe;
namespace L = std::contracts::labels;
constexpr L::empty_label_t e = L::empty_label;
static_assert(L::assertion_control_object<L::empty_label_t>);
static_assert(L::assertion_control_object<L::review_t>);
static_assert(!L::assertion_control_object<int>);

// REQUIRES: contracts-libcxx
