// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -verify %s

// P3098 x constant evaluation: a postcondition capture in a constexpr function.
// The capture snapshots a constant parameter, so the predicate is
// constant-evaluable and this call should compile cleanly.
//
// A capture that is not bound in the constant-evaluation call frame ICEs the
// constant evaluator (ExprConstant.cpp, "missing value for local variable").
// GCC currently rejects this program with "contract condition is not
// constant"; that gap remains open there.

// expected-no-diagnostics
constexpr int good(int x) post [old = x] (r: r == old) { return x; }
constexpr int a = good(5);
