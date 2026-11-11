// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t && %t

// For a non-virtual member call the P3100 null-dereference guard is emitted at
// the call site, on the object expression, so at -O2 the optimizer cannot
// delete it on account of a nonnull `this': `np->f()' and `np->g()' reach the
// handler.
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
__attribute__((noinline)) int mcall() {
  np->f();
  return 0;
}
__attribute__((noinline)) int mcall_reads() { return np->g(); }

int main() {
  if (setjmp(env) == 0)
    mcall();
  if (setjmp(env) == 0)
    mcall_reads();
  if (caught != 2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
