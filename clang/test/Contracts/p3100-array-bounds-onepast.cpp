// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds-onepast.json %libcxx_flags -o %t && %t

// P3100: forming a one-past-the-end address &g[N] is legal and must NOT be
// redirected to &g[0]; a genuinely out-of-range &g[N+k] is redirected.
// Conversely, a *dereference* at exactly one-past (index == bound) IS a
// violation and must be redirected to index 0.

#include <cstdlib>
int g[4] = { 0, 1, 2, 3 };
int h[3] = { 55, 66, 77 };
namespace ign_ns {
  const int* endp() { return &g[4]; }        // one-past: legal, keep as-is
  const int* at(int i) { return &g[i]; }
  int rd(int i) { return h[i]; }             // deref: index == bound is UB
}
__attribute__((noinline)) int opaque(int x) { return x; }

int main() {
  if (ign_ns::endp() != g + 4) std::abort();        // NOT redirected
  if (ign_ns::at(opaque(5)) != g + 0) std::abort();  // out -> &g[0]
  // A dereference at exactly one-past (index == bound) is redirected to index 0
  // (regression: Clang previously missed index == bound for a dereference).
  if (ign_ns::rd(opaque(3)) != h[0]) std::abort();   // deref -> h[0]
}
