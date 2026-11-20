// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A lambda in a postcondition may capture a P3098 capture by copy (or by
// reference); it sees the value captured at the call.  (A mutable lambda
// modifying its copy is ../p3098-mutable-lambda-copies-capture.cpp, fixed as
// CLANG-643.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-lambda-copies-capture.C

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int g(int y) post [old = y] ([=] { return old > 0; }()) { return y; }
int h(int &y) post [old = y] ([&] { return old == 1; }()) { return ++y; }
auto lam = [](int y) post [old = y] ([=] { return old > 0; }()) { return y; };
struct S {
  int m(int y) post [old = y] ([=] { return old > 0; }()) { return y; }
};
template <class T>
T t(T y) post [old = y] ([=] { return old > 0; }()) { return y; }
int d(int y) post [old = y] ([=] { return old > 0; }());
int d(int y) { return y; }

int main() {
  if (g(1) != 1 || violations != 0)
    return 1;
  if (g(-1) != -1 || violations != 1)
    return 2;
  int v = 1;
  if (h(v) != 2 || violations != 1)
    return 3;
  violations = 0;
  if (lam(1) + S().m(1) + t(1) + d(1) != 4 || violations != 0)
    return 4;
  lam(0); S().m(0); t(0); d(0);
  if (violations != 4)
    return 5;
  return 0;
}
