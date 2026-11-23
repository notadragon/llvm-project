// RUN: %clangxx -O0 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-configuration-file=%S/p3100-null.json %libcxx_flags -o %t2 && %t2

// The P3100 null-dereference check tests the object pointer of `->*' with a
// pointer to data member, so `np->*pm' with a null np reaches the handler
// ([expr.mptr.oper] makes it (*np).*pm), at -O0 as at -O2.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-ptrmem.C

#include <contracts>
#include <csetjmp>

static std::jmp_buf env;
static int caught = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++caught;
  std::longjmp(env, 1); // do not continue into the faulting access
}

struct S { int a; int b; };
S *volatile np = nullptr;
int S::*volatile pm = &S::b;

__attribute__((noinline)) int ptm() { return np->*pm; }

int main() {
  if (setjmp(env) == 0)
    ptm();
  if (caught != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
