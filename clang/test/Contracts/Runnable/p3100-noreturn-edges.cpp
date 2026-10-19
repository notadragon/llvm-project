// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-invalid-noreturn \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a [[noreturn]] function that returns
// (ub:dcl.attr.noreturn.eventually.returns) under "enforce", with a handler
// that throws to get back: a return statement, flowing off the end, a
// value-returning function, a GNU noreturn function, a member function and a
// direct call are caught, after the function's locals are destroyed; a
// function leaving by an exception or by longjmp is not, nor is a function
// that is not noreturn.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noreturn-edges.C)

#include <contracts>
#include <csetjmp>
#include <cstdio>
namespace cs = std::contracts;
struct Back {};
static int viols, dtors;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
  std::printf("VIOL dtors=%d\n", dtors);
  throw Back{};
}
struct D {
  ~D() { ++dtors; }
};
static std::jmp_buf env;
[[noreturn]] __attribute__((noinline)) void ret(int x) {
  D d;
  if (x)
    return;
  throw x;
}
[[noreturn]] __attribute__((noinline)) void off(int x) {
  D d;
  if (!x)
    throw x;
}
[[noreturn]] __attribute__((noinline)) int val(int x) {
  if (x)
    return x;
  throw x;
}
__attribute__((noreturn, noinline)) void gnu(int x) {
  if (!x)
    throw x;
}
struct S {
  [[noreturn]] __attribute__((noinline)) void m(int x) {
    if (!x)
      throw x;
  }
};
[[noreturn]] __attribute__((noinline)) void thrower(int x) { throw x; }
[[noreturn]] __attribute__((noinline)) void jumper(int) { std::longjmp(env, 1); }
__attribute__((noinline)) void plain(int) {}
void (*volatile pret)(int) = ret;
void (*volatile poff)(int) = off;
int (*volatile pval)(int) = val;
void (*volatile pgnu)(int) = gnu;
void (*volatile pthrower)(int) = thrower;
void (*volatile pjumper)(int) = jumper;
int main() {
  try {
    pthrower(1);
  } catch (int) {
  }
  if (setjmp(env) == 0)
    pjumper(1);
  plain(1);
  std::printf("viols=%d\n", viols);
  try {
    pret(1);
  } catch (Back &) {
  }
  try {
    poff(1);
  } catch (Back &) {
  }
  try {
    pval(1);
  } catch (Back &) {
  }
  try {
    pgnu(1);
  } catch (Back &) {
  }
  try {
    S().m(1);
  } catch (Back &) {
  }
  try {
    ret(1);
  } catch (Back &) {
  }
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: viols=0
// CHECK-NEXT: VIOL dtors=1
// CHECK-NEXT: VIOL dtors=2
// CHECK-NEXT: VIOL dtors=2
// CHECK-NEXT: VIOL dtors=2
// CHECK-NEXT: VIOL dtors=2
// CHECK-NEXT: VIOL dtors=3
// CHECK-NEXT: viols=6
