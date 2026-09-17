// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -fsyntax-only \
// RUN:   2>&1 | FileCheck %s

// Constant-evaluation observe-violation reporting volume.  KNOWN DIVERGENCE from
// GCC: GCC caps the reported observe violations at 8
// per evaluation and summarises the rest ("and N more contract violation not
// shown"); Clang applies no such cap -- it reports every violation.  This test
// pins Clang's actual behaviour (all nine reported, no summary line).
// (GCC mirror: g++.dg/contracts/cpp26/contract-constexpr-observe-cap.C.)

#include <contracts>

// Nine const-but-false observe violations in one evaluation: Clang reports all
// nine and emits no cap-summary note.
// CHECK-COUNT-9: contract failed during execution of constexpr function
// CHECK-NOT: not shown
constexpr int f(int x) {
  contract_assert(x > 0);
  contract_assert(x > 1);
  contract_assert(x > 2);
  contract_assert(x > 3);
  contract_assert(x > 4);
  contract_assert(x > 5);
  contract_assert(x > 6);
  contract_assert(x > 7);
  contract_assert(x > 8);
  return x;
}

constexpr int bad = f(-1);
