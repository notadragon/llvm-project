// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-shadow-caller-side.json -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: P4021R2 Example 3 is folding's -- divide(num, 0) is always
// false with or without the proof shadow -- and the shadow does not change
// it; divide(num, d) with a d the optimizers know is proven by the shadow,
// and one with an unknown d is not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-example3.C

// Force calling code to check for divide-by-0.
static int divide(int num, int denominator)
  pre(denominator != 0)         // expected-note 2 {{the predicate of the contract assertion}}
{
  return num / denominator;
}

int main() {
  int num = 10;
  int result = divide(num, 0);  // expected-error {{contract predicate is always false}}
  return result;
}

// Not in main: divide(num, 0) makes everything after it undefined, so
// unreachable in the shadow.
int other(int num, int k) {
  int d = k > 1 ? 2 : 5;
  int result = divide(num, d);
  result += divide(num, k);     // expected-error {{contract assertion cannot be proven}}
  return result;
}
