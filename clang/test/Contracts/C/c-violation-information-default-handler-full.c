// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags %s -o %t
// RUN: %t 2>&1 | FileCheck %s --match-full-lines --implicit-check-not={{.}}
// REQUIRES: contracts-libcxx, native

// Under -fcontract-violation-information=full the default violation
// handler prints the function, file, line and comment, as it always has;
// c-violation-information-default-handler.c is the same under none.
// (Clang names the function by its signature, GCC by its identifier.)
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/gcc.dg/contracts/c-violation-information-default-handler-full.c.

void pre_f(int x) _Pre(x > 0) {}
void assert_f(int x) { _ContractAssert(x > 0); }

int main(void) {
  pre_f(0);
  assert_f(0);
  return 0;
}

// CHECK:      contract violation in function void pre_f(int) at {{.*}}c-violation-information-default-handler-full.c:14: x > 0
// CHECK-NEXT: contract violation in function void assert_f(int) at {{.*}}c-violation-information-default-handler-full.c:15: x > 0
