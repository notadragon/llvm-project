// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose %libcxx_flags -o %t -Xclang -verify
// RUN: %t

// P4405: a postcondition with captures (P3098) under prove or diagnose
// initializes no captures -- a capture initializer that would throw is never
// run -- and its predicate is not evaluated; the evaluation is diagnosed
// where its check would be emitted.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-post-captures.C

#include <contracts>

static int captured, evaluated, violations;
int cap(int x) { ++captured; if (x < 0) throw 1; return x; }
bool chk(int c) { ++evaluated; return c > 0; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

struct S { int v; S(int x) : v(x) {} ~S() {} };

int f(int x) post [c = cap(x)] (chk(c)) { return x; }                 // expected-warning {{contract assertion cannot be proven}}
S g(int x) post [s = S(cap(x))] (r: chk(r.v + s.v)) { return S(x); } // expected-warning {{contract assertion cannot be proven}}

int main() {
  f(-1);
  g(-1);
  if (captured != 0 || evaluated != 0 || violations != 0)
    __builtin_abort();
}
