// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -o %t && %t

// A throwing postcondition capture initializer under noexcept_observe: the
// handler runs once with detection_mode::evaluation_exception, and f returns.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/post-capture-noexcept.C

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int handled = 0;
static bool exception_mode = false;

int g(int x) { if (x) throw 1; return x; }
int f(int x) post [c = g(x)] (c == 0) { return x; }

void handle_contract_violation(const contract_violation &v) {
  ++handled;
  exception_mode = v.detection_mode() == detection_mode::evaluation_exception;
}

int main() {
  int r = f(1);
  return r == 1 && handled == 1 && exception_mode ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
