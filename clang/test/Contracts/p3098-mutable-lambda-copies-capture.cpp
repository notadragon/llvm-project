// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A postcondition capture (P3098) is declared inside the contract assertion
// and is not const-ified, so a mutable lambda in the predicate that captures
// it by copy has a member of its type (int) and may modify that copy, also
// in a nested lambda (CLANG-643: Clang made the copy const, "cannot assign
// to variable 'old' with const-qualified type 'const int'").  The capture
// of a const parameter is an int too (deduced as `auto' would be).  A
// non-mutable lambda's copy is const, as anywhere, and the parameter itself
// stays constified.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-lambda-copies-capture.C
// in the gnu_gcc fork (its `k' and `k2').

int k(int y) post [old = y] ([=]() mutable { return ++old > 1; }()) { return y; }
int k2(const int y) post [old = y] ([=]() mutable { return ++old > 1; }() && old == y) {
  return y;
}
int k3(int y) post [old = y] ([=]() mutable { return [=]() mutable { return ++old; }() > 1; }()) {
  return y;
}
int k4(int y) post [old = y] ([=] { return ++old > 1; }()) { return y; } // expected-error {{cannot assign to a variable captured by copy in a non-mutable lambda}}
