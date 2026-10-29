// RUN: %clangxx -O0 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t2 && %t2

// A virtual call through a null pointer is checked for the P3100
// null-dereference assertion at the call site, before the vptr is loaded, so
// `np->v()' reaches the handler.
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

struct S {
  int a;
  int b;
  int arr[4];
  void f() {}
  virtual void v() {}
  int g() { return b; }
};
S *volatile np = nullptr;
__attribute__((noinline)) int vcall() {
  np->v();
  return 0;
}

int main() {
  if (setjmp(env) == 0)
    vcall();
  if (caught != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
