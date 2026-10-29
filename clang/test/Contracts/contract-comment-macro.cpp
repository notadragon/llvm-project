// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// The comment of a contract whose predicate involves macros is the source
// text of the predicate's expansion range, verbatim: from the start of its
// first token, or of the outermost macro invocation that token comes from,
// to the end of its last, or of that token's outermost invocation.  So a
// macro invocation at either end of the predicate, a macro from a header, a
// multi-line invocation, a comment inside an invocation, a string argument
// holding ")", and a pasted token all print as written.  When the whole
// contract is written by a macro, the comment is that macro's invocation as
// written, not the predicate (GCC gives the predicate there; the comment is
// implementation-defined, and this pins Clang's).  A postcondition's comment
// includes its result name.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-macro.C
// (and contract-comment-macro.h) in the gnu_gcc fork has the same shapes.
// GCC also has contract-comment-macro-untracked.C, under
// -ftrack-macro-expansion=0; Clang has no such option (it always tracks
// macro expansions), so it has no mirror here.

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "contract-comment-macro.h"

#define X x
#define ZERO 0
#define Y X
#define NEG(a) ((a) < 0)
#define ID(e) e
#define LHS x <
#define RHS < 0
#define FIRST(a, b) (a)
#define CAT(a, b) a ## b
#define PRE_NEG pre (x < 0)
#define MY_PRE(c) pre (c)
#define MY_POST(c) post (r: c)
#define MY_ASSERT(c) contract_assert (c)
#define WRAP(c) MY_PRE (c)
#define CHK(c) contract_assert ((c) > 0)
#define BOTH(a, b) pre (a && b)
#define TWICE(a) pre (a == -a)

const char *expected;
int violations, mismatches;

void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++violations;
  if (std::strcmp(v.comment(), expected) != 0) {
    ++mismatches;
    std::fprintf(stderr, "expected \"%s\", got \"%s\"\n", expected,
                 v.comment());
  }
}

// The predicate's own parentheses written outside any macro.
int begins_in(int x) pre (X < 0) { return x; }
int begins_in_fn(int x) pre (NEG (x) && x > 9) { return x; }
int ends_in(int x) pre (x < ZERO) { return x; }
int ends_in_fn(int x) pre (x > 9 || NEG (x)) { return x; }
int two(int x) pre (X < ZERO) { return x; }
int two_fn(int x) pre (NEG (x) || NEG (x)) { return x; }
int one_fn(int x) pre (NEG (x)) { return x; }
int first_half(int x) pre (LHS 0) { return x; }
int second_half(int x) pre (x RHS) { return x; }
int header(int x) pre (HX < 0) { return x; }
int nested(int x) pre (Y < 0) { return x; }
int in_argument(int x) pre (ID (x) < 0) { return x; }
int whole_argument(int x) pre (ID (x < 0)) { return x; }
int multi_line(int x) pre (NEG (
                             x)) { return x; }
int with_comment(int x) pre (NEG (x /* why */)) { return x; }
int string_argument(int x) pre (FIRST (x < 0, ")")) { return x; }
int pasted(int xy) pre (CAT (x, y) < 0) { return xy; }

// The whole contract from a macro.
int body(int x) PRE_NEG { return x; }
int argument(int x) MY_PRE (x < 0) { return x; }
int argument_two(int x) MY_PRE (X < ZERO) { return x; }
int argument_fn(int x) MY_PRE (NEG (x)) { return x; }
int argument_wrapped(int x) WRAP (x < 0) { return x; }
int argument_post(int x) MY_POST (r < 0) { return x; }

// Tokens from more than one argument, or one argument used twice.
int two_arguments(int x) BOTH (x > 0, x < 5) { return x; }
int argument_twice(int x) TWICE (x) { return x; }

struct S {
  int member(int x) pre (NEG (x)) { return x; }
  int member_argument(int x) MY_PRE (X < ZERO) { return x; }
};

void check(const char *e, int (*f)(int)) {
  expected = e;
  f(5);
}

int main() {
  check("X < 0", begins_in);
  check("NEG (x) && x > 9", begins_in_fn);
  check("x < ZERO", ends_in);
  check("x > 9 || NEG (x)", ends_in_fn);
  check("X < ZERO", two);
  check("NEG (x) || NEG (x)", two_fn);
  check("NEG (x)", one_fn);
  check("LHS 0", first_half);
  check("x RHS", second_half);
  check("HX < 0", header);
  check("Y < 0", nested);
  check("ID (x) < 0", in_argument);
  check("ID (x < 0)", whole_argument);
  check("NEG (\n                             x)", multi_line);
  check("NEG (x /* why */)", with_comment);
  check("FIRST (x < 0, \")\")", string_argument);
  check("CAT (x, y) < 0", pasted);

  check("PRE_NEG", body);
  check("MY_PRE (x < 0)", argument);
  check("MY_PRE (X < ZERO)", argument_two);
  check("MY_PRE (NEG (x))", argument_fn);
  check("WRAP (x < 0)", argument_wrapped);
  check("MY_POST (r < 0)", argument_post);

  check("BOTH (x > 0, x < 5)", two_arguments);
  check("TWICE (x)", argument_twice);

  S s;
  expected = "NEG (x)";
  s.member(5);
  expected = "MY_PRE (X < ZERO)";
  s.member_argument(5);

  int x = 5;
  expected = "X < ZERO";
  contract_assert(X < ZERO);
  expected = "MY_ASSERT (x < 0)";
  MY_ASSERT (x < 0);
  expected = "CHK (-x)";
  CHK (-x);

  if (violations != 30 || mismatches != 0)
    std::abort();
}
