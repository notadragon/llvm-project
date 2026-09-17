// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -O2 -c -o %t.o

// P3100: with -fcontracts-p3100 and no configuration, the flow-off-end implicit
// assertion defaults to "assume" -- today's behavior (no check emitted).  This
// just verifies the default path compiles cleanly.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-implicit-flow-off-default.C)

int classify(int x) {
  if (x > 0)
    return x * 2;
  // x <= 0: control flows off the end (assume: left as UB, no check).
}
