// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -O2 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe,ub:basic.align.object.alignment:noexcept_observe,ub:expr.mul.div.by.zero:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// P3100: the effects of the contract-violation handler reached from a native
// implicit check -- the null-dereference, alignment and division-by-zero
// checks -- are seen by the code after the check when optimizing: the check
// calls the handler like any other call, so `reports`, a static only the
// handler changes, is read again after it.  Each form is counted once at
// -O2, also when the check is inlined into the caller (`maddr`).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-handler-effects-inlined.C,
// GCC-805.)

#include <contracts>
#include <cstdio>

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
}

struct S { int m; };
inline int *maddr(S *sp) { return &sp->m; }
static volatile int vz = 0;

int main() {
  S *volatile vs = nullptr;
  S *s = vs;
  int before = reports;
  int *volatile r = maddr(s);
  std::printf("null %d\n", reports - before);

  alignas(int) char buf[8] = {};
  int *pi = (int *)(buf + 1 + vz);
  before = reports;
  volatile int y = *pi;
  std::printf("align %d\n", reports - before);

  int z = vz;
  before = reports;
  volatile int q = 7 / z;
  std::printf("div %d\n", reports - before);
  (void)r;
  (void)y;
  (void)q;
  return 0;
}

// CHECK: null 1
// CHECK-NEXT: align 1
// CHECK-NEXT: div 1
