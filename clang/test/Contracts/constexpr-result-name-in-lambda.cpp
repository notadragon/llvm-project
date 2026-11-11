// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A lambda in a postcondition that uses the result name by implicit capture
// can be constant-evaluated: it reads the capture, not a result slot in its
// own frame.  The explicit-capture shape is result-name-explicit-capture.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-in-lambda.C

constexpr int fl(const int x) post(r : [&] { return r == x; }()) { return x; }
static_assert(fl(4) == 4);
