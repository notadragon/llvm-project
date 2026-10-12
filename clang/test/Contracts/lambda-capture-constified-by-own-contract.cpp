// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A by-reference capture first made from one of the lambda's own contract
// assertions -- its postcondition, or a contract_assert before any other use
// -- is not const: the lambda's body may modify the entity through it.  A
// use in the predicate is const whatever the capture is (the last line).
// (CLANG-660: the capture was made const, so the order of the uses decided
// whether the body could assign.)

int f() {
  int t = 3;
  auto l = [&](const int u) post(u > t) { t = 5; return u; };
  auto l2 = [&](const int u) { contract_assert(u > t); t = 5; return u; };
  auto l3 = [&](const auto u) post(u > t) { t = 5; return u; };
  auto l4 = [&](const int u) { t = 5; contract_assert(u > t); return u; };
  auto bad = [&t] pre(++t > 0) {}; // expected-error {{cannot assign to}}
  return l(4) + l2(4) + l3(4) + l4(4);
}
