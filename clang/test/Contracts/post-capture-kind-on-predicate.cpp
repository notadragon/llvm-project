// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A postcondition that has captures reports assertion_kind::post for a false
// predicate and for a predicate that throws; post_capture is only for an
// exception from constructing or destroying a capture (P3098).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/post-capture-kind-on-predicate.C

#include <contracts>

using std::contracts::assertion_kind;
using std::contracts::contract_violation;

static assertion_kind last{};
static int n = 0;
void handle_contract_violation(const contract_violation &v) {
  last = v.kind();
  ++n;
}

struct Bad { Bad(int) { throw 1; } bool ok() const { return true; } };
bool boom(int) { throw 2; }
int f(const int x) post [c = x] (c < 0) { return x; }        // predicate false
int h(const int x) post [c = x] (boom(c)) { return x; }      // predicate throws
int g(const int x) post [c = Bad(x)] (c.ok()) { return x; }  // capture throws

int main() {
  f(1);
  if (n != 1 || last != assertion_kind::post)
    __builtin_abort();
  h(1);
  if (n != 2 || last != assertion_kind::post)
    __builtin_abort();
  g(1);   // control: this one is post_capture
  if (n != 3 || last != assertion_kind::post_capture)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
