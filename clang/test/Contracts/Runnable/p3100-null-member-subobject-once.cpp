// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -Wno-tautological-undefined-compare \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s

// P3100: one null dereference is reported once under "observe" when a member
// subobject is used through a null object pointer.  The check is on the
// outermost object pointer -- the member access checks p -- and the use of
// the member does not check the member's address again, which is null too
// when the offset is 0: a reference bound to it, a member function called
// on it (directly, through a pointer to member function, virtually, or on a
// member array's first element), a read or write of the member of p->*pm
// or of an element or member of it.  Two controls: a member at a nonzero
// offset, reported once before as after.  A null dereference that is read
// faults after the handler returns; the fault is caught as SIGSEGV and
// resumed with siglongjmp, so one run counts the reports of every form.
// (Regression test for CLANG-742, which reported each twice.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-member-subobject-once.C.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct I {
  int m;
  __attribute__((noinline)) int f() { return 7; }
};
struct S { int arr[4]; I s; };
struct T { I s; int n; };
struct A { I arr[2]; };
struct M { int m; int n; };
struct V { virtual int vf() { return 1; } };
struct W { V v; };
struct B { int b; };
struct VB : virtual B { int x; };
struct R { int &r; };

#define NI __attribute__((noinline))
NI int &ref_mm(M *p) { return p->m; }
NI int &ref_m(T *p) { return p->s.m; }
NI I &ref_s(T *p) { return p->s; }
NI int &ref_pmm(M *p, int M::*pm) { return p->*pm; }
NI int &ref_pm(T *p, I T::*ps) { return (p->*ps).m; }
NI int &ref_parr(S *p, int (S::*pa)[4]) { return (p->*pa)[0]; }
NI int &ref_pa(int (*pa)[4]) { return (*pa)[0]; }
NI int &ref_vb(VB *p, int B::*pm) { return p->*pm; }
NI int agg_r(M *p) { R x{p->m}; return &x.r != nullptr; }
NI int &ref_n(M *p) { return p->n; }
NI int call_s(T *p) { return p->s.f(); }
NI int call_ps(T *p, I T::*ps) { return (p->*ps).f(); }
NI int call_star(T *p) { return (*p).s.f(); }
NI int call_pmf(T *p, int (I::*pmf)()) { return (p->s.*pmf)(); }
NI int call_arr(A *p) { return p->arr->f(); }
NI int call_wv(W *p) { return p->v.vf(); }
NI int rd_arr(S *p) { return p->arr[0]; }
NI int rd_star(S *p) { return (*p).arr[0]; }
NI int rd_parr(S *p, int (S::*pa)[4]) { return (p->*pa)[0]; }
NI int rd_pm(M *p, int M::*pm) { return p->*pm; }
NI int rd_cpm(M *const p, int M::*pm) { return p->*pm; }
NI int rd_psm(T *p, I T::*ps) { return (p->*ps).m; }
NI int rd_spm(T *p, int I::*pm) { return p->s.*pm; }
NI int rd_dpm(M *q, int M::*pm) { return (*q).*pm; }
NI int rd_qpm(M *q, int M::*pm, int i) { return q[i].*pm; }
NI int rd_vb(VB *p, int B::*pm) { return p->*pm; }
NI void wr_pm(M *p, int M::*pm) { p->*pm = 3; }
NI void inc_pm(M *p, int M::*pm) { ++(p->*pm); }
NI int rd_pmn(M *p, int M::*pm) { return p->*pm; }

#define RUN(name, ...)                                                         \
  do {                                                                         \
    reports = 0;                                                               \
    if (!sigsetjmp(env, 1))                                                    \
      (void)(__VA_ARGS__);                                                     \
    std::printf("%s %d\n", #name, reports);                                    \
  } while (0)

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::signal(SIGSEGV, on_segv);
  RUN(ref_mm, &ref_mm(nullptr));
  RUN(ref_m, &ref_m(nullptr));
  RUN(ref_s, &ref_s(nullptr));
  RUN(ref_pmm, &ref_pmm(nullptr, &M::m));
  RUN(ref_pm, &ref_pm(nullptr, &T::s));
  RUN(ref_parr, &ref_parr(nullptr, &S::arr));
  RUN(ref_pa, &ref_pa(nullptr));
  RUN(ref_vb, &ref_vb(nullptr, &B::b));
  RUN(agg_r, agg_r(nullptr));
  RUN(ref_n, &ref_n(nullptr));
  RUN(call_s, call_s(nullptr));
  RUN(call_ps, call_ps(nullptr, &T::s));
  RUN(call_star, call_star(nullptr));
  RUN(call_pmf, call_pmf(nullptr, &I::f));
  RUN(call_arr, call_arr(nullptr));
  RUN(call_wv, call_wv(nullptr));
  RUN(rd_arr, rd_arr(nullptr));
  RUN(rd_star, rd_star(nullptr));
  RUN(rd_parr, rd_parr(nullptr, &S::arr));
  RUN(rd_pm, rd_pm(nullptr, &M::m));
  RUN(rd_cpm, rd_cpm(nullptr, &M::m));
  RUN(rd_psm, rd_psm(nullptr, &T::s));
  RUN(rd_spm, rd_spm(nullptr, &I::m));
  RUN(rd_dpm, rd_dpm(nullptr, &M::m));
  RUN(rd_qpm, rd_qpm(nullptr, &M::m, 0));
  RUN(rd_vb, rd_vb(nullptr, &B::b));
  RUN(wr_pm, wr_pm(nullptr, &M::m));
  RUN(inc_pm, inc_pm(nullptr, &M::m));
  RUN(rd_pmn, rd_pmn(nullptr, &M::n));
  return 0;
}
// CHECK: ref_mm 1
// CHECK-NEXT: ref_m 1
// CHECK-NEXT: ref_s 1
// CHECK-NEXT: ref_pmm 1
// CHECK-NEXT: ref_pm 1
// CHECK-NEXT: ref_parr 1
// CHECK-NEXT: ref_pa 1
// CHECK-NEXT: ref_vb 1
// CHECK-NEXT: agg_r 1
// CHECK-NEXT: ref_n 1
// CHECK-NEXT: call_s 1
// CHECK-NEXT: call_ps 1
// CHECK-NEXT: call_star 1
// CHECK-NEXT: call_pmf 1
// CHECK-NEXT: call_arr 1
// CHECK-NEXT: call_wv 1
// CHECK-NEXT: rd_arr 1
// CHECK-NEXT: rd_star 1
// CHECK-NEXT: rd_parr 1
// CHECK-NEXT: rd_pm 1
// CHECK-NEXT: rd_cpm 1
// CHECK-NEXT: rd_psm 1
// CHECK-NEXT: rd_spm 1
// CHECK-NEXT: rd_dpm 1
// CHECK-NEXT: rd_qpm 1
// CHECK-NEXT: rd_vb 1
// CHECK-NEXT: wr_pm 1
// CHECK-NEXT: inc_pm 1
// CHECK-NEXT: rd_pmn 1
