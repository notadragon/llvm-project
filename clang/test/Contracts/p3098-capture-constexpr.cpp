// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -verify %s

// P3098 x constant evaluation: a postcondition capture in a constexpr function.
// The capture snapshots a constant parameter, so the predicate is
// constant-evaluable and this call should compile cleanly.
//
// BUG-8 (Clang, fixed): captures used to be unusable during constant
// evaluation -- Clang ICE'd in the constant evaluator (ExprConstant.cpp
// "missing value for local variable"). GCC still rejects this with
// "contract condition is not constant" (BUG-8 remains open on GCC). See
// wg21 testing-gap-catalogue.md section 10.

// expected-no-diagnostics
constexpr int good(int x) post [old = x] (r: r == old) { return x; }
constexpr int a = good(5);
