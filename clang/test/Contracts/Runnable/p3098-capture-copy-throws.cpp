// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3098: a throwing copy constructor on a capture is a capture-initialization
// exception, and so a post_capture violation.  This path is only reachable
// once the capture is genuinely copy-initialized; a bit-copy cannot throw.
// (GCC mirror: g++.dg/contracts/cpp26/p3098-capture-copy-throws.C)
//
// p3098-except-init.cpp declares a throwing copy constructor but never calls
// it -- its capture is a prvalue, so the throw comes from the converting
// constructor instead.  The parameter here is taken by reference so that the
// only copy is the capture's.

#include <contracts>
#include <cstdio>

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++handler_count;
  if (v.kind() != std::contracts::assertion_kind::post_capture) __builtin_abort();
}

static int predicate_count = 0;
bool count_predicate() { ++predicate_count; return true; }

struct Throwing {
  int v;
  Throwing(int x) : v(x) {}
  Throwing(const Throwing &) { throw 42; }
  ~Throwing() {}
};

int f(const Throwing &p) post [q = p] (count_predicate()) { return p.v; }

int main() {
  Throwing t(10);
  int result = f(t);

  if (handler_count != 1) __builtin_abort();
  if (result != 10) __builtin_abort();
  // The predicate is skipped: the capture it would read was never built.
  if (predicate_count != 0) __builtin_abort();

  std::printf("PASS\n");
}
