// RUN: %clang -std=c23 -fcontracts-p3850 -fno-contracts-p3100 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// REQUIRES: contracts-libcxx, native

// DECISIONS.md E179 (1): -fcontracts-p3850 turns C contracts on itself, so a
// C link with it pulls in libcontracts even when -fno-contracts-p3100 turns
// off the P3100 option it implies: no explicit -l in the test
// (contracts-implicit-link.c for -fcontracts-p4299; %libcxx_flags is only a
// library search path here).  The link line is p3100-c-link-driver.c.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p3850-link.c

int f(int x) _Pre(x > 0) { return x; }

int main(void) { return f(1) == 1 ? 0 : 1; }
