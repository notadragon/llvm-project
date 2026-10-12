// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -fcontract-evaluation-semantic=quick_enforce -O2 -Wall -Wextra -Wuninitialized -Wsometimes-uninitialized -emit-llvm -o /dev/null -verify %s
// expected-no-diagnostics

// A potentially-throwing predicate checked under quick_enforce draws no
// warning about an uninitialized value.  GCC warned about a compiler
// temporary ("'D.1234' may be used uninitialized"): its check stores the
// predicate's result in a temporary inside a try block and the catch (...)
// handler's reaction did not set it (GCC-689 in gnu_gcc's bug-reports/).
// Clang's uninitialized-use warnings are front-end analyses of the source,
// which has no such temporary, so this is a regression guard.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-quick-enforce-throwing-predicate-uninit.C

bool positive(int);

void pre_fn(int x) pre(positive(x)) {}

int post_fn(int x) post(r: positive(r)) { return x; }

void assert_fn(int x) { contract_assert(positive(x)); }

int main() {
  pre_fn(1);
  assert_fn(2);
  return post_fn(3);
}
