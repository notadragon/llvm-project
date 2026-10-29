// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.v
// RUN: %t.v | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.v2
// RUN: %t.v2 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.n
// RUN: %t.n | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.n2
// RUN: %t.n2 | FileCheck %s

// P3100: a read through a null pointer of a member of a virtual base of an
// element at a constant subscript, `vbp[1].v`, converts `vbp + 1` to the
// virtual base, reading the base's offset through it: reported once
// (.access, DECISIONS.md E94 (b); the check tests `vbp`, E97 (c)), with
// -fsanitize=vptr routed to the contract-violation handler too and
// without it, at -O0 and -O2.  A fault after the report is caught as
// SIGSEGV and resumed with siglongjmp.
// (GCC mirror: g++.dg/ubsan/open-bug-p3100-null-vptr-vbase-subscript.C,
// GCC-861's watch: not reported there with -fsanitize=vptr.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>

static int reports;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reports;
}

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct V {
  virtual void h() {}
  int v;
};
struct VB : virtual V { int w; };
__attribute__((noinline)) int vbsubrd(VB *p) { return p[1].v; }

int main() {
  std::signal(SIGSEGV, on_segv);
  VB *volatile vbp = nullptr;
  int before = reports;
  if (!sigsetjmp(env, 1))
    vbsubrd(vbp);
  std::printf("vbsubrd %d\n", reports - before);
  return 0;
}

// CHECK: vbsubrd 1
