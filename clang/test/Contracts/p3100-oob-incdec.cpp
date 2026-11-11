// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-oob-incdec.json %libcxx_flags -o %t && %t

// Under P3100 the array bounds check reports an index equal to the bound for
// `a[N]++' and `++a[N]' as for a plain store: the operand of an increment or
// decrement is accessed, so the one-past address is not allowed; taking it
// with `&' still is, by the bounds check.  (It is an indirection past the end,
// ub:expr.unary.dereference.invalid.known, which the configuration leaves at
// the default.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-oob-incdec.C

#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

struct S { int a[4]; int guard; };

int main(int argc, char **) {
  int i = argc + 3;   // 4 == bound, not a constant
  {
    S s{};
    s.a[i] = 1;       // control
    if (reported != 1 || s.guard != 0)
      __builtin_abort();
  }
  {
    S s{};
    s.a[i]++;
    if (reported != 2 || s.guard != 0)
      __builtin_abort();
  }
  {
    S s{};
    ++s.a[i];
    if (reported != 3 || s.guard != 0)
      __builtin_abort();
  }
  {
    S s{};
    int *p = &s.a[i]; // one-past address: not a violation
    (void)p;
    if (reported != 3)
      __builtin_abort();
  }
}

// REQUIRES: contracts-libcxx, native
