// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-null-deref-quick.json %libcxx_flags -o %t && %t

// P3100: &*p does not access the object, so no null-dereference assertion fires
// there even under quick_enforce -- taking the address of a dereference is
// well-defined and must not be instrumented.  (Reuses the quick config.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-addr-of.C)

#include <contracts>

int *addr_of_deref(int *p) { return &*p; }   // no load/store: not a deref

int main() {
  int *p = nullptr;
  // &*p folds to p; no access occurs, so quick_enforce must not trap here.
  return addr_of_deref(p) == nullptr ? 0 : 1;
}
