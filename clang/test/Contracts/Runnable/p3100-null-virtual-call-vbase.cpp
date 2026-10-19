// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND

// P3100: a call through a null pointer to a class with a virtual base of a
// function of that base, virtual or not, converts the pointer to the
// virtual base, reading the base's offset through it: the .access leaf of
// ub:expr.unary.dereference.nullptr (DECISIONS.md E94 (b)), reported once
// at -O0 and -O2, and not under .indirection alone.  The forms: a virtual
// call through `->`, through `*`, on an element at a constant subscript,
// a qualified call, a non-virtual call, and a virtual call on a member of
// a class with a virtual base.  A fault after the report is caught as
// SIGSEGV and resumed with siglongjmp, so one run counts the reports of
// every form.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-virtual-call-vbase*.C,
// GCC-859: a virtual call was reported twice there at -O1 and above.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

#define NI __attribute__((noinline))
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
struct VB : virtual V { int w; };
struct HV { int x; VB vb; };
NI void vbh(VB *p) { p->h(); }
NI void vbstar(VB *p) { (*p).h(); }
NI void vbsub(VB *p) { p[1].h(); }
NI void vbq(VB *p) { p->V::h(); }
NI void vbnv(VB *p) { p->nv(); }
NI void vbmem(HV *hp) { hp->vb.h(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  VB *vbp = reinterpret_cast<VB *>(nv);
  HV *hvp = reinterpret_cast<HV *>(nv);
  COUNT("vbh", vbh(vbp));
  COUNT("vbstar", vbstar(vbp));
  COUNT("vbsub", vbsub(vbp));
  COUNT("vbq", vbq(vbp));
  COUNT("vbnv", vbnv(vbp));
  COUNT("vbmem", vbmem(hvp));
  return 0;
}

// ALL: vbh 1
// ALL-NEXT: vbstar 1
// ALL-NEXT: vbsub 1
// ALL-NEXT: vbq 1
// ALL-NEXT: vbnv 1
// ALL-NEXT: vbmem 1
// IND: vbh 0
// IND-NEXT: vbstar 0
// IND-NEXT: vbsub 0
// IND-NEXT: vbq 0
// IND-NEXT: vbnv 0
// IND-NEXT: vbmem 0
