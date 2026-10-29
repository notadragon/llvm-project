// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A postcondition capture is initialized in lexical order with the
// preconditions, and is destroyed on the way out when a lexically later
// precondition's handler throws (f), or a later postcondition's capture
// initializer throws and the post_capture handler throws (f2).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/capture-leak-later-throw.C

#include <contracts>

static int live = 0;
struct Good {
  Good() { ++live; }
  Good(const Good &) { ++live; }
  ~Good() { --live; }
};
struct Bad {
  Bad() { throw 1; }
};

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw 7;
}

int f(int i) post [g = Good()] (true) pre(i > 0) { return i; }

int f2(int i) post [g = Good()] (true) post [b = Bad()] (true) { return i; }

int main() {
  try { f(-1); } catch (int) {}
  if (live != 0)
    __builtin_abort();
  try { f2(1); } catch (int) {}
  if (live != 0)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
