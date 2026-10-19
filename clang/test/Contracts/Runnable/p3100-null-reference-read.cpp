// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// P3100: a glvalue that is already a reference -- a reference variable,
// parameter, member or capture -- indirects through nothing, so a read or
// write through it is not checked again (DECISIONS.md E92 (a)): one null
// dereference, `*p` bound to the reference, is reported once, under
// .indirection, when the reference is then read, stored to, has a member or
// an element read, is copied or assigned from, is converted to a virtual
// base or has a member of one read, or names the object of `.*`.  A pointer
// formed from the reference is a pointer: an indirection through it is
// checked (`S *q = &r; q->m`, twice under the group, the second under
// .access).  A non-null pointer is not reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-reference-read*.C, whose
// header has these forms in this order and reports each the same.)

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
struct S {
  int m;
  int a[2];
};
struct B { int b; };
struct V : virtual B { int v; };

NI int ptrmem_ref(S *sp, int S::*pm) { S &r = *sp; return r.*pm; }
NI int ptrmem_param(S &r, int S::*pm) { return r.*pm; }
NI void ptrmem_store(S &r, int S::*pm) { r.*pm = 2; }
NI void store_ref(int *p) { int &r = *p; r = 1; }
NI void store_param(int &r) { r = 1; }
NI int read_param(S &r) { return r.m; }
NI int array_param(S &r) { return r.a[1]; }
NI void scopy_param(S &r) { S s2 = r; (void)s2; }
NI void sassign_param(S &r, S &s) { s = r; }
NI int vbase_param(V &v) { B &b = v; return b.b; }
NI int vbase_read(V &v) { return v.b; }
NI int member_ref(S *sp) { struct R { S &r; } x{*sp}; return x.r.m; }
NI int capture_ref(S *sp) { S &r = *sp; return [&r] { return r.m; }(); }
// A pointer formed from a reference is a pointer: an indirection through it
// is checked.
NI int ptr_from_ref(S *sp) { S &r = *sp; S *q = &r; return q->m; }

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
  int *volatile vp = nullptr;
  int *ip = vp;
  S *sp = reinterpret_cast<S *>(vp);
  V *vbp = reinterpret_cast<V *>(vp);
  int i = 0;
  S s{};
  V v;
  COUNT("ptrmem_ref", ptrmem_ref(sp, &S::m));
  COUNT("ptrmem_param", ptrmem_param(*sp, &S::m));
  COUNT("ptrmem_store", ptrmem_store(*sp, &S::m));
  COUNT("store_ref", store_ref(ip));
  COUNT("store_param", store_param(*ip));
  COUNT("read_param", read_param(*sp));
  COUNT("array_param", array_param(*sp));
  COUNT("scopy_param", scopy_param(*sp));
  COUNT("sassign_param", sassign_param(*sp, s));
  COUNT("vbase_param", vbase_param(*vbp));
  COUNT("vbase_read", vbase_read(*vbp));
  COUNT("member_ref", member_ref(sp));
  COUNT("capture_ref", capture_ref(sp));
  COUNT("ptr_from_ref", ptr_from_ref(sp));
  COUNT("nonnull", ptrmem_ref(&s, &S::m); ptrmem_param(s, &S::m);
        ptrmem_store(s, &S::m); store_ref(&i); store_param(i); read_param(s);
        array_param(s); scopy_param(s); sassign_param(s, s); vbase_param(v);
        vbase_read(v); member_ref(&s); capture_ref(&s); ptr_from_ref(&s));
  return 0;
}

// ALL: ptrmem_ref 1
// ALL-NEXT: ptrmem_param 1
// ALL-NEXT: ptrmem_store 1
// ALL-NEXT: store_ref 1
// ALL-NEXT: store_param 1
// ALL-NEXT: read_param 1
// ALL-NEXT: array_param 1
// ALL-NEXT: scopy_param 1
// ALL-NEXT: sassign_param 1
// ALL-NEXT: vbase_param 1
// ALL-NEXT: vbase_read 1
// ALL-NEXT: member_ref 1
// ALL-NEXT: capture_ref 1
// ALL-NEXT: ptr_from_ref 2
// ALL-NEXT: nonnull 0

// IND: ptrmem_ref 1
// IND-NEXT: ptrmem_param 1
// IND-NEXT: ptrmem_store 1
// IND-NEXT: store_ref 1
// IND-NEXT: store_param 1
// IND-NEXT: read_param 1
// IND-NEXT: array_param 1
// IND-NEXT: scopy_param 1
// IND-NEXT: sassign_param 1
// IND-NEXT: vbase_param 1
// IND-NEXT: vbase_read 1
// IND-NEXT: member_ref 1
// IND-NEXT: capture_ref 1
// IND-NEXT: ptr_from_ref 1
// IND-NEXT: nonnull 0

// ACC: ptrmem_ref 0
// ACC-NEXT: ptrmem_param 0
// ACC-NEXT: ptrmem_store 0
// ACC-NEXT: store_ref 0
// ACC-NEXT: store_param 0
// ACC-NEXT: read_param 0
// ACC-NEXT: array_param 0
// ACC-NEXT: scopy_param 0
// ACC-NEXT: sassign_param 0
// ACC-NEXT: vbase_param 0
// ACC-NEXT: vbase_read 0
// ACC-NEXT: member_ref 0
// ACC-NEXT: capture_ref 0
// ACC-NEXT: ptr_from_ref 1
// ACC-NEXT: nonnull 0
