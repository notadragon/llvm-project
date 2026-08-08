// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-config-kind.json %libcxx_flags -o %t && %t

// P3595 x config: "kind" matching selects semantics per contract kind.  Only
// kind:"pre" was previously covered; this exercises kind:"post" and
// kind:"contract_assert".  The config sends post and assert to ignore and lets
// pre fall through to the observe catch-all.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-config-kind.C)

#include <contracts>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

void has_pre(int x) pre(x > 0) { }                 // catch-all -> observe
int has_post(int x) post(r: r > 0) { return x; }   // kind:post -> ignore
void has_assert(int x) { contract_assert(x > 0); } // kind:contract_assert -> ignore

int main() {
  has_pre(-1);      // observe -> handler called
  if (violations != 1) std::abort();
  has_post(-1);     // ignore -> no handler
  if (violations != 1) std::abort();
  has_assert(-1);   // ignore -> no handler
  if (violations != 1) std::abort();
}
