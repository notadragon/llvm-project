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
// P3100: an array element copied through a null pointer -- an element of the
// pointer (`s = sp[1]`, `S s2 = sp[1]`, by value, returned, moved) or of a
// member array (`s = hp->sa[1]`, the first element at the object's address)
// -- is the copy's source under a qualification conversion.  A copy of a
// class reads the element, so the null pointer is reported once under
// .access (`sp` itself for `sp[i]`, as for an accessed `sp[i]` of a scalar);
// a copy of an empty class reads no object, .indirection (DECISIONS.md E86
// (a)); a copy of an empty virtual base of the element reads the base's
// offset through it, .access (DECISIONS.md E94 (b)).  A non-null pointer is
// not reported.  Then GCC's extra forms: a scalar element read or stored to
// through a reference cast (`static_cast<int &>(p[1])`), .access.
// (CLANG-773: the qualification conversion hid the subscript from
// EmitCheckedLValue: `sp[1]` copied was not checked, an element of a member
// array was .indirection, and the first element was reported twice.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-subscript-copy*.C, whose
// header has these forms in this order.  GCC reports a class copied from
// `sp[1]` on `sp` too; it checks the element's address for an empty one,
// so `esub_from`, `esub_copy` and `esub_arg` are not reported there: the
// CLANG-745 divergence, which gnu_gcc GCC-825 narrows to non-constant
// subscripts.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <utility>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct E {};
struct S { int a, b; };
struct VE : virtual E { int v; };
struct HS { int x; S sa[2]; };
struct H0 { S sa[2]; int x; };
struct HE0 { E ea[2]; int x; };
struct HV { int x; VE va[2]; };

#define NI __attribute__((noinline))
NI void sbyval(S) {}
NI void ebyval(E) {}
NI void ssub_from(S *sp, S &s) { s = sp[1]; }
NI void ssub_copy(S *sp) { S s2 = sp[1]; (void)s2; }
NI void ssub_arg(S *sp) { sbyval(sp[1]); }
NI S ssub_ret(S *sp) { return sp[1]; }
NI void ssub_move(S *sp) { S s2 = std::move(sp[1]); (void)s2; }
NI void ssub0_from(S *sp, S &s) { s = sp[0]; }
NI void sarray_from(HS *hp, S &s) { s = hp->sa[1]; }
NI void sarray_copy(HS *hp) { S s2 = hp->sa[1]; (void)s2; }
NI void sarray0_from(H0 *hp, S &s) { s = hp->sa[0]; }
NI void sarray0_copy(H0 *hp) { S s2 = hp->sa[0]; (void)s2; }
NI void esub_from(E *ep, E &e) { e = ep[1]; }
NI void esub_copy(E *ep) { E e2 = ep[1]; (void)e2; }
NI void esub_arg(E *ep) { ebyval(ep[1]); }
NI void earray0_from(HE0 *hp, E &e) { e = hp->ea[0]; }
NI void earray0_copy(HE0 *hp) { E e2 = hp->ea[0]; (void)e2; }
NI void vsub_from(VE *vp, E &e) { e = vp[1]; }
NI void hvarray_from(HV *hp, E &e) { e = hp->va[1]; }
// GCC's: a scalar element read or stored to through a reference cast.
NI int isub_cast(int *p) { return static_cast<int &>(p[1]); }
NI void isub_cast_store(int *p) { static_cast<int &>(p[1]) = 2; }

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
  S *sp = reinterpret_cast<S *>(vp);
  E *ep = reinterpret_cast<E *>(vp);
  int *ip = vp;
  VE *vep = reinterpret_cast<VE *>(vp);
  HS *hsp = reinterpret_cast<HS *>(vp);
  H0 *h0p = reinterpret_cast<H0 *>(vp);
  HE0 *he0p = reinterpret_cast<HE0 *>(vp);
  HV *hvp = reinterpret_cast<HV *>(vp);
  S s, sa[2]{};
  E e, ea[2];
  VE va[2];
  HS hs{};
  H0 h0{};
  HE0 he0{};
  HV hv{};
  int ia[2]{};
  COUNT("ssub_from", ssub_from(sp, s));
  COUNT("ssub_copy", ssub_copy(sp));
  COUNT("ssub_arg", ssub_arg(sp));
  COUNT("ssub_ret", ssub_ret(sp));
  COUNT("ssub_move", ssub_move(sp));
  COUNT("ssub0_from", ssub0_from(sp, s));
  COUNT("sarray_from", sarray_from(hsp, s));
  COUNT("sarray_copy", sarray_copy(hsp));
  COUNT("sarray0_from", sarray0_from(h0p, s));
  COUNT("sarray0_copy", sarray0_copy(h0p));
  COUNT("esub_from", esub_from(ep, e));
  COUNT("esub_copy", esub_copy(ep));
  COUNT("esub_arg", esub_arg(ep));
  COUNT("earray0_from", earray0_from(he0p, e));
  COUNT("earray0_copy", earray0_copy(he0p));
  COUNT("vsub_from", vsub_from(vep, e));
  COUNT("hvarray_from", hvarray_from(hvp, e));
  COUNT("nonnull", ssub_from(sa, s); ssub_copy(sa); ssub_arg(sa); ssub_ret(sa);
        ssub_move(sa); ssub0_from(sa, s); sarray_from(&hs, s);
        sarray_copy(&hs); sarray0_from(&h0, s); sarray0_copy(&h0);
        esub_from(ea, e); esub_copy(ea); esub_arg(ea); earray0_from(&he0, e);
        earray0_copy(&he0); vsub_from(va, e); hvarray_from(&hv, e));
  COUNT("isub_cast", isub_cast(ip));
  COUNT("isub_cast_store", isub_cast_store(ip));
  COUNT("nonnull_extra", isub_cast(ia); isub_cast_store(ia));
  return 0;
}

// ALL: ssub_from 1
// ALL-NEXT: ssub_copy 1
// ALL-NEXT: ssub_arg 1
// ALL-NEXT: ssub_ret 1
// ALL-NEXT: ssub_move 1
// ALL-NEXT: ssub0_from 1
// ALL-NEXT: sarray_from 1
// ALL-NEXT: sarray_copy 1
// ALL-NEXT: sarray0_from 1
// ALL-NEXT: sarray0_copy 1
// ALL-NEXT: esub_from 1
// ALL-NEXT: esub_copy 1
// ALL-NEXT: esub_arg 1
// ALL-NEXT: earray0_from 1
// ALL-NEXT: earray0_copy 1
// ALL-NEXT: vsub_from 1
// ALL-NEXT: hvarray_from 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: isub_cast 1
// ALL-NEXT: isub_cast_store 1
// ALL-NEXT: nonnull_extra 0

// IND: ssub_from 0
// IND-NEXT: ssub_copy 0
// IND-NEXT: ssub_arg 0
// IND-NEXT: ssub_ret 0
// IND-NEXT: ssub_move 0
// IND-NEXT: ssub0_from 0
// IND-NEXT: sarray_from 0
// IND-NEXT: sarray_copy 0
// IND-NEXT: sarray0_from 0
// IND-NEXT: sarray0_copy 0
// IND-NEXT: esub_from 1
// IND-NEXT: esub_copy 1
// IND-NEXT: esub_arg 1
// IND-NEXT: earray0_from 1
// IND-NEXT: earray0_copy 1
// IND-NEXT: vsub_from 0
// IND-NEXT: hvarray_from 0
// IND-NEXT: nonnull 0
// IND-NEXT: isub_cast 0
// IND-NEXT: isub_cast_store 0
// IND-NEXT: nonnull_extra 0

// ACC: ssub_from 1
// ACC-NEXT: ssub_copy 1
// ACC-NEXT: ssub_arg 1
// ACC-NEXT: ssub_ret 1
// ACC-NEXT: ssub_move 1
// ACC-NEXT: ssub0_from 1
// ACC-NEXT: sarray_from 1
// ACC-NEXT: sarray_copy 1
// ACC-NEXT: sarray0_from 1
// ACC-NEXT: sarray0_copy 1
// ACC-NEXT: esub_from 0
// ACC-NEXT: esub_copy 0
// ACC-NEXT: esub_arg 0
// ACC-NEXT: earray0_from 0
// ACC-NEXT: earray0_copy 0
// ACC-NEXT: vsub_from 1
// ACC-NEXT: hvarray_from 1
// ACC-NEXT: nonnull 0
// ACC-NEXT: isub_cast 1
// ACC-NEXT: isub_cast_store 1
// ACC-NEXT: nonnull_extra 0
