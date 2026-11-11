// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all0
// RUN: %t.all0 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc0
// RUN: %t.acc0 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -w -fsanitize=null -fsanitize-recover=null \
// RUN:   %libcxx_flags -o %t.san0
// RUN: %t.san0 2>&1 | FileCheck %s --check-prefix=SAN
// RUN: %clangxx -std=c++26 %s -w -O2 -fsanitize=null -fsanitize-recover=null \
// RUN:   %libcxx_flags -o %t.san2
// RUN: %t.san2 2>&1 | FileCheck %s --check-prefix=SAN

// A call through a null pointer to a member function returning a class
// with a non-trivial copy constructor and destructor by value: checked
// like any member call, by the P3100 null check -- once, under
// .indirection for a non-virtual function, under .access for a virtual
// one, devirtualized through a `final` class or not (DECISIONS.md E102
// (a), E110 (a)) -- and by -fsanitize=null alone.  Each is paired with
// the same call returning int, the control.  Then gnu_gcc's, the class
// returned: on an element at a constant subscript (the P3100 check tests
// p, DECISIONS.md E97 (c); stock -fsanitize=null tests the element's
// address, p + 1, and reports nothing), on a member, on a member of a
// `final` class, a qualified virtual call, on `*p`, through a reference
// bound to `*p`; and a non-null control.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-member-call-class-return*.C
// (GCC-854's P3100 half, fixed under DECISIONS.md E113 (b)), whose header
// has these forms in this order, and
// gcc/testsuite/g++.dg/ubsan/open-bug-null-member-call-class-return.C, the
// watch test of the stock half (deferred: stock GCC's -fsanitize=null does
// not see the call returning the class).

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

#define NI __attribute__((noinline))
struct R {
  R() {}
  R(const R &) {}
  ~R() {}
  int r;
};
struct S {
  R f() { return R(); }
  int g() { return 0; }
};
struct V {
  virtual R f() { return R(); }
  virtual int g() { return 0; }
};
struct F final {
  virtual R f() { return R(); }
  virtual int g() { return 0; }
};
NI void scls(S *p) { p->f(); }
NI void sint(S *p) { p->g(); }
NI void vcls(V *p) { p->f(); }
NI void vint(V *p) { p->g(); }
NI void fcls(F *p) { p->f(); }
NI void fint(F *p) { p->g(); }
// gnu_gcc's (GCC-854), the class returned: a call on an element at a
// constant subscript, on a member, on a member of a `final` class
// (devirtualized), a qualified virtual call, a call on `*p` and through a
// reference bound to `*p`.
struct H {
  int x;
  S m;
  F fm;
};
NI void subcls(S *p) { p[1].f(); }
NI void memcls(H *p) { p->m.f(); }
NI void fmemcls(H *p) { p->fm.f(); }
NI void qcls(V *p) { p->V::f(); }
NI void starcls(S *p) { (*p).f(); }
NI void refcls(S &r) { r.f(); }
NI void refc(S *p) { refcls(*p); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    std::printf("%s\n", name);                                                 \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  std::signal(SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  COUNT("scls", scls(reinterpret_cast<S *>(nv)));
  COUNT("sint", sint(reinterpret_cast<S *>(nv)));
  COUNT("vcls", vcls(reinterpret_cast<V *>(nv)));
  COUNT("vint", vint(reinterpret_cast<V *>(nv)));
  COUNT("fcls", fcls(reinterpret_cast<F *>(nv)));
  COUNT("fint", fint(reinterpret_cast<F *>(nv)));
  COUNT("subcls", subcls(reinterpret_cast<S *>(nv)));
  COUNT("memcls", memcls(reinterpret_cast<H *>(nv)));
  COUNT("fmemcls", fmemcls(reinterpret_cast<H *>(nv)));
  COUNT("qcls", qcls(reinterpret_cast<V *>(nv)));
  COUNT("starcls", starcls(reinterpret_cast<S *>(nv)));
  COUNT("refc", refc(reinterpret_cast<S *>(nv)));
  S s;
  H h;
  COUNT("nonnull", scls(&s); memcls(&h); fmemcls(&h); starcls(&s));
  return 0;
}

// ALL: scls 1
// ALL: sint 1
// ALL: vcls 1
// ALL: vint 1
// ALL: fcls 1
// ALL: fint 1
// ALL: subcls 1
// ALL: memcls 1
// ALL: fmemcls 1
// ALL: qcls 1
// ALL: starcls 1
// ALL: refc 1
// ALL: nonnull 0

// IND: scls 1
// IND: sint 1
// IND: vcls 0
// IND: vint 0
// IND: fcls 0
// IND: fint 0
// IND: subcls 1
// IND: memcls 1
// IND: fmemcls 0
// IND: qcls 1
// IND: starcls 1
// IND: refc 1
// IND: nonnull 0

// ACC: scls 0
// ACC: sint 0
// ACC: vcls 1
// ACC: vint 1
// ACC: fcls 1
// ACC: fint 1
// ACC: subcls 0
// ACC: memcls 0
// ACC: fmemcls 1
// ACC: qcls 0
// ACC: starcls 0
// ACC: refc 0
// ACC: nonnull 0

// SAN: {{^}}scls{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'S'
// SAN: {{^}}sint{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'S'
// SAN: {{^}}vcls{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'V'
// SAN: {{^}}vint{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'V'
// SAN: {{^}}fcls{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'F'
// SAN: {{^}}fint{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'F'
// SAN: {{^}}subcls{{$}}
// SAN-NEXT: {{^}}subcls 0{{$}}
// SAN: {{^}}memcls{{$}}
// SAN-NEXT: runtime error: member access within null pointer of type 'H'
// SAN: {{^}}fmemcls{{$}}
// SAN-NEXT: runtime error: member access within null pointer of type 'H'
// SAN: {{^}}qcls{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'V'
// SAN: {{^}}starcls{{$}}
// SAN-NEXT: runtime error: member call on null pointer of type 'S'
// SAN: {{^}}refc{{$}}
// SAN-NEXT: runtime error: reference binding to null pointer of type 'S'
// SAN: {{^}}nonnull{{$}}
// SAN-NEXT: {{^}}nonnull 0{{$}}
