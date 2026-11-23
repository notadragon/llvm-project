// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %S/../constexpr-contract-every-problem.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %S/../constexpr-contract-array-bound.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fsyntax-only -verify -verify-ignore-unexpected=note -fexperimental-new-constant-interpreter %S/../constexpr-contract-observe-warning-once.cpp
// XFAIL: *

// OPEN BUG (CLANG-617).  Clang only: GCC has one constant evaluator.
//
// The bytecode constant interpreter does not treat contract problems met
// during constant evaluation as DECISIONS.md M7-M9 require, which the tree
// evaluator does since CLANG-611/614: a not-constant predicate fails the
// evaluation instead of being reported once it completes, only the first
// problem is reported, and an observe warning can repeat.  These are the tree
// evaluator's own tests, run under the interpreter.  When they pass, the XFAIL
// becomes an XPASS, which lit reports, and that is the signal to close the bug.
