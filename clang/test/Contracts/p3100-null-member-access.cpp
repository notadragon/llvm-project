// RUN: %clangxx -O0 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t2 && %t2

// The P3100 null-dereference guard is emitted for class member access
// through `->', so `np->b', `np->arr[i]' and `&np->b' with a null np reach the
// handler.  [expr.ref] makes `p->b' `(*p).b'.
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
__attribute__((noinline)) int member() { return np->b; }
__attribute__((noinline)) int arrmem() {
  volatile int i = 2;
  return np->arr[i];
}
__attribute__((noinline)) int arrow_addr() { return &np->b != (int *)0; }

int main() {
  if (setjmp(env) == 0)
    member();
  if (setjmp(env) == 0)
    arrmem();
  if (setjmp(env) == 0)
    arrow_addr();
  if (caught != 3)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
