// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// evaluation_semantic::unspecified (0) can be put in an
// evaluation_semantic_set and asked about in constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/evaluation-semantic-set-unspecified.C

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

constexpr evaluation_semantic_set s(evaluation_semantic::unspecified);
static_assert(
    !evaluation_semantic_set::all().contains(evaluation_semantic::unspecified));

// REQUIRES: contracts-libcxx
