// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 %libcxx_flags -Wno-unused-command-line-argument -I%S/../../../libcxx/modules --precompile %t/probe.cppm -o %t/probe.pcm
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3850 %libcxx_flags -Wno-unused-command-line-argument -fmodule-file=probe=%t/probe.pcm -fsyntax-only -Xclang -verify %t/use.cpp

// The std module's contracts partition (libcxx/modules/std/contracts.inc)
// exports every public std::contracts name, including evaluation_semantic_set,
// violation_handled, is_nonthrowing and the handle_*_contract_violation
// functions.  The test exports contracts.inc from a small module rather than
// building all of std.
//
// Mirror: gcc/testsuite/g++.dg/modules/std-module-contract-names.C

//--- probe.cppm
module;
#include <contracts>
export module probe;
#include "std/contracts.inc"

//--- use.cpp
// expected-no-diagnostics
import probe;
std::contracts::evaluation_semantic e = std::contracts::evaluation_semantic::observe;
std::contracts::evaluation_semantic_set s;
std::contracts::violation_handled h = std::contracts::violation_handled::handled;
bool b = std::contracts::is_nonthrowing(e);
void f() {
  std::contracts::handle_observed_contract_violation("x");
  std::contracts::handle_enforced_contract_violation("x");
  std::contracts::handle_quick_enforced_contract_violation("x");
}

// REQUIRES: contracts-libcxx
