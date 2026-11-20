// RUN: %clangxx -O0 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t2 && %t2

// An accessed `p[3]' with a null p (which is `*(p + 3)') is checked for the
// P3100 null-dereference assertion on the pointer being subscripted, not on
// the element's address.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-shapes.C
// in the gnu_gcc fork.

#include <contracts>
#include <csetjmp>

static std::jmp_buf env;
static int caught = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++caught;
  std::longjmp(env, 1); // do not continue into the faulting access
}

__attribute__((noinline)) int subscript() {
  int *volatile p = nullptr;
  return p[3];
}

int main() {
  if (setjmp(env) == 0)
    subscript();
  if (caught != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
