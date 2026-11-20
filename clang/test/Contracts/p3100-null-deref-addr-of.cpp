// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:quick_enforce %libcxx_flags -o %t && %t

// P3100: &*p does not access the object, so the .access leaf of the
// null-dereference check does not fire there even under quick_enforce; the
// indirection is the .indirection leaf's (CLANG-753, DECISIONS.md E86;
// Runnable/p3100-null-unaccessed-indirection.cpp).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-addr-of.C)

#include <contracts>

int *addr_of_deref(int *p) { return &*p; }   // no load/store: not an access

int main() {
  int *p = nullptr;
  // &*p folds to p; no access occurs, so .access must not trap here.
  return addr_of_deref(p) == nullptr ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
