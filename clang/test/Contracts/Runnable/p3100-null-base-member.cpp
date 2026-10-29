// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND

// P3100: a member of a base class used through a null pointer is one null
// dereference, reported once under "observe" and under the leaf of the use,
// as a member of the class itself is: the object is checked before its
// conversion to the base, which for a virtual base loads the base's offset
// through the object's vptr -- an access of the object, so every use of a
// virtual base's member is the .access leaf -- and for a base at a nonzero
// offset yields an address that is not null.  The same holds when the
// object expression is a member subobject (p->d.b, checked once, on p), a
// subscript (q[i].b, checked on q), a pointer loaded from memory
// ((*pp)->b), the object of a .* (((*p).*pmf)(), (*p).*pm), and for
// (p->*pmf)() and &(p->*pm) with a virtual base.  A base at offset 0
// (rd_first) is the control.  A null dereference that is read faults after
// the handler returns; the fault is caught as SIGSEGV and resumed with
// siglongjmp, so one run counts the reports of every form.  (Regression
// test for CLANG-747, which missed every form but the control, or reported
// it twice.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-base-member{,-access,
// -indirection}.C; GCC reports each form once, under the same leaf.)

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

struct B0 { int z; };
struct B {
  int b;
  __attribute__((noinline)) int f() { return 7; }
};
struct D : B0, B { int x; };
struct D1 : B { int x; };
struct VB : virtual B { int x; };
struct H { D1 d; D dd; VB v; };

#define NI __attribute__((noinline))
// A virtual base: every use reads the vptr, so every form is .access.
NI int v_rd(VB *p) { return p->b; }
NI void v_wr(VB *p) { p->b = 1; }
NI int *v_addr(VB *p) { return &p->b; }
NI int &v_ref(VB *p) { return p->b; }
NI int v_call(VB *p) { return p->f(); }
NI int v_rd_star(VB *p) { return (*p).b; }
NI int v_call_star(VB *p) { return (*p).f(); }
NI B &v_bind(VB *p) { return *p; }
NI int v_cast(VB *p) { return static_cast<B &>(*p).b; }
NI int v_member(H *p) { return p->v.b; }
NI int v_sub(VB *q, int i) { return q[i].b; }
// A base at a nonzero offset: the leaf of the use.
NI int n_rd(D *p) { return p->b; }
NI int *n_addr(D *p) { return &p->b; }
NI int &n_ref(D *p) { return p->b; }
NI int n_call(D *p) { return p->f(); }
NI int n_rd_star(D *p) { return (*p).b; }
NI int n_call_star(D *p) { return (*p).f(); }
NI B &n_bind(D *p) { return *p; }
NI int n_loaded(D **pp) { return (*pp)->b; }
NI int n_sub(D *q, int i) { return q[i].b; }
// A member subobject whose base's member is used.
NI int m_rd(H *p) { return p->d.b; }
NI int m_call(H *p) { return p->d.f(); }
NI int &m_ref(H *p) { return p->d.b; }
NI int m_rd_off(H *p) { return p->dd.b; }
// Pointers to members of a base.
NI int n_pmf_arrow(D *p, int (B::*pmf)()) { return (p->*pmf)(); }
NI int n_pmf_dot(D *p, int (B::*pmf)()) { return ((*p).*pmf)(); }
NI int n_pm_dot(D *p, int B::*pm) { return (*p).*pm; }
NI int v_pmf_arrow(VB *p, int (B::*pmf)()) { return (p->*pmf)(); }
NI int v_pmf_dot(VB *p, int (B::*pmf)()) { return ((*p).*pmf)(); }
NI int v_pm_dot(VB *p, int B::*pm) { return (*p).*pm; }
NI int *v_pm_addr(VB *p, int B::*pm) { return &(p->*pm); }
// The control: a base at offset 0.
NI int rd_first(D1 *p) { return p->b; }

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
  D *n = nullptr;
  RUN(v_rd, v_rd(nullptr));
  RUN(v_wr, v_wr(nullptr));
  RUN(v_addr, v_addr(nullptr));
  RUN(v_ref, &v_ref(nullptr));
  RUN(v_call, v_call(nullptr));
  RUN(v_rd_star, v_rd_star(nullptr));
  RUN(v_call_star, v_call_star(nullptr));
  RUN(v_bind, &v_bind(nullptr));
  RUN(v_cast, v_cast(nullptr));
  RUN(v_member, v_member(nullptr));
  RUN(v_sub, v_sub(nullptr, 0));
  RUN(n_rd, n_rd(nullptr));
  RUN(n_addr, n_addr(nullptr));
  RUN(n_ref, &n_ref(nullptr));
  RUN(n_call, n_call(nullptr));
  RUN(n_rd_star, n_rd_star(nullptr));
  RUN(n_call_star, n_call_star(nullptr));
  RUN(n_bind, &n_bind(nullptr));
  RUN(n_loaded, n_loaded(&n));
  RUN(n_sub, n_sub(nullptr, 0));
  RUN(m_rd, m_rd(nullptr));
  RUN(m_call, m_call(nullptr));
  RUN(m_ref, &m_ref(nullptr));
  RUN(m_rd_off, m_rd_off(nullptr));
  RUN(n_pmf_arrow, n_pmf_arrow(nullptr, &B::f));
  RUN(n_pmf_dot, n_pmf_dot(nullptr, &B::f));
  RUN(n_pm_dot, n_pm_dot(nullptr, &B::b));
  RUN(v_pmf_arrow, v_pmf_arrow(nullptr, &B::f));
  RUN(v_pmf_dot, v_pmf_dot(nullptr, &B::f));
  RUN(v_pm_dot, v_pm_dot(nullptr, &B::b));
  RUN(v_pm_addr, v_pm_addr(nullptr, &B::b));
  RUN(rd_first, rd_first(nullptr));
  return 0;
}
// ALL: v_rd 1
// ALL-NEXT: v_wr 1
// ALL-NEXT: v_addr 1
// ALL-NEXT: v_ref 1
// ALL-NEXT: v_call 1
// ALL-NEXT: v_rd_star 1
// ALL-NEXT: v_call_star 1
// ALL-NEXT: v_bind 1
// ALL-NEXT: v_cast 1
// ALL-NEXT: v_member 1
// ALL-NEXT: v_sub 1
// ALL-NEXT: n_rd 1
// ALL-NEXT: n_addr 1
// ALL-NEXT: n_ref 1
// ALL-NEXT: n_call 1
// ALL-NEXT: n_rd_star 1
// ALL-NEXT: n_call_star 1
// ALL-NEXT: n_bind 1
// ALL-NEXT: n_loaded 1
// ALL-NEXT: n_sub 1
// ALL-NEXT: m_rd 1
// ALL-NEXT: m_call 1
// ALL-NEXT: m_ref 1
// ALL-NEXT: m_rd_off 1
// ALL-NEXT: n_pmf_arrow 1
// ALL-NEXT: n_pmf_dot 1
// ALL-NEXT: n_pm_dot 1
// ALL-NEXT: v_pmf_arrow 1
// ALL-NEXT: v_pmf_dot 1
// ALL-NEXT: v_pm_dot 1
// ALL-NEXT: v_pm_addr 1
// ALL-NEXT: rd_first 1
// ACC: v_rd 1
// ACC-NEXT: v_wr 1
// ACC-NEXT: v_addr 1
// ACC-NEXT: v_ref 1
// ACC-NEXT: v_call 1
// ACC-NEXT: v_rd_star 1
// ACC-NEXT: v_call_star 1
// ACC-NEXT: v_bind 1
// ACC-NEXT: v_cast 1
// ACC-NEXT: v_member 1
// ACC-NEXT: v_sub 1
// ACC-NEXT: n_rd 1
// ACC-NEXT: n_addr 0
// ACC-NEXT: n_ref 0
// ACC-NEXT: n_call 0
// ACC-NEXT: n_rd_star 1
// ACC-NEXT: n_call_star 0
// ACC-NEXT: n_bind 0
// ACC-NEXT: n_loaded 1
// ACC-NEXT: n_sub 1
// ACC-NEXT: m_rd 1
// ACC-NEXT: m_call 0
// ACC-NEXT: m_ref 0
// ACC-NEXT: m_rd_off 1
// ACC-NEXT: n_pmf_arrow 0
// ACC-NEXT: n_pmf_dot 0
// ACC-NEXT: n_pm_dot 1
// ACC-NEXT: v_pmf_arrow 1
// ACC-NEXT: v_pmf_dot 1
// ACC-NEXT: v_pm_dot 1
// ACC-NEXT: v_pm_addr 1
// ACC-NEXT: rd_first 1
// IND: v_rd 0
// IND-NEXT: v_wr 0
// IND-NEXT: v_addr 0
// IND-NEXT: v_ref 0
// IND-NEXT: v_call 0
// IND-NEXT: v_rd_star 0
// IND-NEXT: v_call_star 0
// IND-NEXT: v_bind 0
// IND-NEXT: v_cast 0
// IND-NEXT: v_member 0
// IND-NEXT: v_sub 0
// IND-NEXT: n_rd 0
// IND-NEXT: n_addr 1
// IND-NEXT: n_ref 1
// IND-NEXT: n_call 1
// IND-NEXT: n_rd_star 0
// IND-NEXT: n_call_star 1
// IND-NEXT: n_bind 1
// IND-NEXT: n_loaded 0
// IND-NEXT: n_sub 0
// IND-NEXT: m_rd 0
// IND-NEXT: m_call 1
// IND-NEXT: m_ref 1
// IND-NEXT: m_rd_off 0
// IND-NEXT: n_pmf_arrow 1
// IND-NEXT: n_pmf_dot 1
// IND-NEXT: n_pm_dot 0
// IND-NEXT: v_pmf_arrow 0
// IND-NEXT: v_pmf_dot 0
// IND-NEXT: v_pm_dot 0
// IND-NEXT: v_pm_addr 0
// IND-NEXT: rd_first 0
