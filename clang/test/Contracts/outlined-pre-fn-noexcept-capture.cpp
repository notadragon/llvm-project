// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontracts-p4298 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t

// A throwing capture copy under -fcontracts-p4298 whose violation handler
// throws propagates the handler's exception to the caller.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/outlined-pre-fn-noexcept-capture.C

#include <contracts>

struct Boom {
  int v;
  Boom(int i) : v(i) {}
  Boom(const Boom &) { throw 42; }
};

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.detection_mode() ==
      std::contracts::detection_mode::evaluation_exception)
    ++handled;
  throw 7;
}

int f(const Boom b) post [c = b] (c.v > 0) { return b.v; }

int main() {
  try {
    f(Boom(1));
  } catch (int e) {
    return (e == 7 && handled == 1) ? 0 : 1;
  }
  return 1;
}

// REQUIRES: contracts-libcxx, native
