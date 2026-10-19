// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t
// REQUIRES: contracts-libcxx, native

// The comment of a contract whose predicate spans several lines is the
// source text of the predicate, verbatim: every line break and every blank
// is kept, including a line holding nothing but blanks, two such lines in a
// row, an empty line, and a continuation line that is not indented.  Covers
// the four places a predicate is parsed: a precondition, a postcondition,
// contract_assert, and a member function's contracts, which are parsed at
// the end of the class.  A postcondition's comment includes its result
// name.  GCC ICEd on a line of only blanks, and reads each line break and
// the blanks after it as one space; the comment is implementation-defined,
// and this pins Clang's.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-blank-line.C
// in the gnu_gcc fork has the same shapes.

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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

// A line of two spaces.
int pre_spaces(int x) pre(x
  
  > 0) { return x; }

// A line of a tab.
int pre_tab(int x) pre(x
	
	> 0) { return x; }

// Spaces and a tab, and two blank lines in a row.
int pre_mixed(int x) pre(x
 	 
  
  > 0) { return x; }

// An empty line.
int pre_empty(int x) pre(x

  > 0) { return x; }

// A continuation line that is not indented.
int pre_flush(int x) pre(x
> 0) { return x; }

int post_spaces(int x) post(r: r
  
  > 0) { return x; }

struct S {
  int member(int x) pre(x
  
    > 0) { return x; }
};

int main() {
  expected = "x\n  \n  > 0";
  pre_spaces(-1);
  expected = "x\n\t\n\t> 0";
  pre_tab(-1);
  expected = "x\n \t \n  \n  > 0";
  pre_mixed(-1);
  expected = "x\n\n  > 0";
  pre_empty(-1);
  expected = "x\n> 0";
  pre_flush(-1);
  expected = "x\n  \n    > 0";
  S().member(-1);

  expected = "r: r\n  \n  > 0";
  post_spaces(-1);

  int x = -1;
  expected = "x < 0\n  \n                  && x > 0";
  contract_assert(x < 0
  
                  && x > 0);

  if (violations != 8 || mismatches != 0)
    std::abort();
}
