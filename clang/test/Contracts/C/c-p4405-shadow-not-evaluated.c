// RUN: %clang -O2 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %libcxx_flags %s -o %t && %t
// REQUIRES: contracts-libcxx, native

// P4405 stage 3 in C: with the proof shadow at -O2 the predicate is never
// evaluated at run time, proven or not.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-not-evaluated.c
// (whose ++evaluated in a predicate Clang rejects, the variable being const
// there; a call does the increment instead)

static int evaluated;
__attribute__((noinline)) static int touch(int v) { ++evaluated; return v; }
__attribute__((noinline)) static int bump(void) { return ++evaluated; }

__attribute__((noinline)) int f(int x)
  _Pre(touch(x > 0))
{
  if (x > 0)
    _ContractAssert(touch(x > 0) && x > 0);
  _ContractAssert(touch(0));
  _ContractAssert(bump() > 0);
  return x;
}

int main(void) {
  if (f(1) + f(-1) != 0 || evaluated != 0)
    __builtin_abort();
  return 0;
}
