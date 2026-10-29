// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-overflow-folded.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-overflow-folded.json %libcxx_flags -o %t2 && %t2

// The P3100 signed-overflow check sees `x + 1 > x' and `x * 4 / 4', which an
// optimizer that assumes no overflow would fold away: each overflow is
// reported, and the result is the wrapped value.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-overflow-folded.C

#include <contracts>
#include <climits>

static int calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++calls;
}

__attribute__((noinline)) bool gt(int x) { return x + 1 > x; }
__attribute__((noinline)) int muldiv(int x) { return x * 4 / 4; }

int main() {
  if (gt(INT_MAX) || calls != 1)
    __builtin_abort();
  if (muldiv(INT_MAX) != -1 || calls != 2)
    __builtin_abort();
}
