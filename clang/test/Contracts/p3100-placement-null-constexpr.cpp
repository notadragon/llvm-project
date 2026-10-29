// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3100 %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:enforce
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3100 %libcxx_flags \
// RUN:   -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P3100: a placement new on a non-null pointer is a constant expression, and
// one on a null pointer is rejected by the evaluator -- configured or not,
// whatever the new-expression initializes (an empty class too).  GCC rejects
// it as undefined where the check is configured and, for the empty class,
// accepts it where it is not (gnu_gcc GCC-779, a stock miss, deferred; watch
// test g++.dg/contracts/cpp26/open-bug-constexpr-placement-new-null-empty.C).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-null-constexpr.C)

#include <new>
struct E {};
constexpr int good() {
  int x = 0;
  int *p = new (&x) int(3);
  return *p;
}
static_assert(good() == 3);
constexpr int bad() {
  int *p = nullptr;
  new (p) int(3); // expected-note {{construction of dereferenced null pointer is not allowed in a constant expression}}
  return 0;
}
constexpr int b = bad(); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'bad()'}}
constexpr int bad_empty() {
  E *p = nullptr;
  new (p) E; // expected-note {{construction of dereferenced null pointer is not allowed in a constant expression}}
  return 0;
}
constexpr int be = bad_empty(); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'bad_empty()'}}
