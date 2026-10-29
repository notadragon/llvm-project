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
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: a trivial copy or move of an empty class through a null pointer
// accesses no object, so the indirection is reported once under .indirection
// (DECISIONS.md E86 (a); Runnable/p3100-null-empty-class-copy.cpp has the
// first forms): passing or returning `*ep` by value, list-initializing
// from it, copying from a reference cast of it, move-assigning from it, in
// a template; an empty non-virtual base of `*dp` -- at offset 0 or not, or
// at a nonzero offset where its address is `dp` plus the offset -- copied,
// assigned from, moved from, passed or returned by value, stored to through
// a reference cast or `dp->E::operator=(e)`, its address taken, a reference
// bound to it, discarded; an empty member of `*hp`, or an element of an
// array of them, copied, moved from, passed by value or stored to (a
// subscript with side effects still evaluated); an empty base of a member
// of `*hp` copied.  Copying an empty virtual base, or discarding a
// conversion to one, is the .access leaf: one of `*vp` reads the base's
// offset through the pointer, and one of a member object, whose offset is
// known, is a conversion to a virtual base all the same (DECISIONS.md E94
// (b); Runnable/p3100-null-vbase-member-bind.cpp has more).  A non-null
// pointer is not reported.
// (CLANG-768: the empty non-virtual bases and the store to the empty member
// were reported under .access.  CLANG-771: `earray_store` was .access;
// CLANG-772: `hvmember_disc` was .indirection.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-empty-class-copy-extra.h,
// these forms in this order; its first eight are also GCC's extra forms of
// p3100-null-empty-class-copy.h.)

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
struct A { int a; };
struct DE : A, E {};
struct DE0 : E { int b; };
struct VE : virtual E { int v; };
struct HE { int x; E e; };
struct HD { int x; DE d; };
// GCC's: an empty base at a nonzero offset (a member of type E is at offset
// 0), an array of empty members, a member with an empty virtual base.
struct F0 { E e; int i; };
struct DEn : F0, E {};
struct HA { int x; E ea[2]; };
struct HV { int x; VE ve; };

#define NI __attribute__((noinline))
NI void ebyval(E) {}
// GCC's extra forms.
NI void earg(E *ep) { ebyval(*ep); }
NI E eret(E *ep) { return *ep; }
NI void ebrace(E *ep) { E e2{*ep}; (void)e2; }
NI void ecast(E *ep) { E e2 = static_cast<E &>(*ep); (void)e2; }
NI void emove_from(E *ep, E &e) { e = std::move(*ep); }
NI void ebase_from(DE *dp, E &e) { e = *dp; }
NI void evbase_from(VE *vp, E &e) { e = *vp; }
template <class T> NI void tcopy(T *p) { T t = *p; (void)t; }
// Clang's extra forms.
NI void ebase_copy(DE *dp) { E e2 = *dp; (void)e2; }
NI void ebase_arg(DE *dp) { ebyval(*dp); }
NI E ebase_ret(DE *dp) { return *dp; }
NI void ebase_store(DE *dp, E &e) { static_cast<E &>(*dp) = e; }
NI void ebase_call(DE *dp, E &e) { dp->E::operator=(e); }
NI void ebase_move(DE *dp, E &e) { e = std::move(*dp); }
NI void ebase0_from(DE0 *dp, E &e) { e = *dp; }
NI void evbase_copy(VE *vp) { E e2 = *vp; (void)e2; }
NI void emember_copy(HE *hp) { E e2 = hp->e; (void)e2; }
NI void emember_arg(HE *hp) { ebyval(hp->e); }
NI void emember_store(HE *hp, E &e) { hp->e = e; }
NI void hdmember_base(HD *hp, E &e) { e = hp->d; }
// GCC's extra forms.
NI void ebasen_from(DEn *dp, E &e) { e = *dp; }
NI void ebasen_copy(DEn *dp) { E e2 = *dp; (void)e2; }
NI void ebasen_move(DEn *dp, E &e) { e = std::move(*dp); }
NI void ebasen_store(DEn *dp, E &e) { static_cast<E &>(*dp) = e; }
NI void ebasen_call(DEn *dp, E &e) { dp->E::operator=(e); }
NI E *ebasen_addr(DEn *dp) { return &static_cast<E &>(*dp); }
NI E *ebasen_bind(DEn *dp) { E &r = *dp; return &r; }
NI void ebasen_disc(DEn *dp) { static_cast<E &>(*dp); }
NI void emember_move(HE *hp) { E e2 = std::move(hp->e); (void)e2; }
NI void emember_move_from(HE *hp, E &e) { e = std::move(hp->e); }
NI void earray_copy(HA *hp) { E e2 = hp->ea[1]; (void)e2; }
NI void earray_from(HA *hp, E &e) { e = hp->ea[1]; }
NI void earray_store(HA *hp, E &e) { hp->ea[1] = e; }
NI int earray_side(HA *hp, E &e) { int i = 0; e = hp->ea[i++]; return i; }
NI void hvmember_from(HV *hp, E &e) { e = hp->ve; }
NI void hvmember_copy(HV *hp) { E e2 = hp->ve; (void)e2; }
NI void hvmember_disc(HV *hp) { static_cast<E &>(hp->ve); }

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
  E *ep = reinterpret_cast<E *>(vp);
  DE *dep = reinterpret_cast<DE *>(vp);
  DE0 *de0p = reinterpret_cast<DE0 *>(vp);
  VE *vep = reinterpret_cast<VE *>(vp);
  HE *hep = reinterpret_cast<HE *>(vp);
  HD *hdp = reinterpret_cast<HD *>(vp);
  DEn *denp = reinterpret_cast<DEn *>(vp);
  HA *hap = reinterpret_cast<HA *>(vp);
  HV *hvp = reinterpret_cast<HV *>(vp);
  E e;
  DE de{};
  DE0 de0{};
  VE ve;
  HE he{};
  HD hd{};
  DEn den{};
  HA ha{};
  HV hv{};
  COUNT("earg", earg(ep));
  COUNT("eret", eret(ep));
  COUNT("ebrace", ebrace(ep));
  COUNT("ecast", ecast(ep));
  COUNT("emove_from", emove_from(ep, e));
  COUNT("ebase_from", ebase_from(dep, e));
  COUNT("evbase_from", evbase_from(vep, e));
  COUNT("tcopy", tcopy(ep));
  COUNT("ebase_copy", ebase_copy(dep));
  COUNT("ebase_arg", ebase_arg(dep));
  COUNT("ebase_ret", ebase_ret(dep));
  COUNT("ebase_store", ebase_store(dep, e));
  COUNT("ebase_call", ebase_call(dep, e));
  COUNT("ebase_move", ebase_move(dep, e));
  COUNT("ebase0_from", ebase0_from(de0p, e));
  COUNT("evbase_copy", evbase_copy(vep));
  COUNT("emember_copy", emember_copy(hep));
  COUNT("emember_arg", emember_arg(hep));
  COUNT("emember_store", emember_store(hep, e));
  COUNT("hdmember_base", hdmember_base(hdp, e));
  COUNT("nonnull", earg(&e); eret(&e); emove_from(&e, e); ebase_from(&de, e);
        evbase_from(&ve, e); tcopy(&e); ebase_copy(&de); ebase_store(&de, e);
        ebase_call(&de, e); ebase_move(&de, e); ebase0_from(&de0, e);
        evbase_copy(&ve); emember_copy(&he); emember_store(&he, e);
        hdmember_base(&hd, e));
  COUNT("ebasen_from", ebasen_from(denp, e));
  COUNT("ebasen_copy", ebasen_copy(denp));
  COUNT("ebasen_move", ebasen_move(denp, e));
  COUNT("ebasen_store", ebasen_store(denp, e));
  COUNT("ebasen_call", ebasen_call(denp, e));
  COUNT("ebasen_addr", ebasen_addr(denp));
  COUNT("ebasen_bind", ebasen_bind(denp));
  COUNT("ebasen_disc", ebasen_disc(denp));
  COUNT("emember_move", emember_move(hep));
  COUNT("emember_move_from", emember_move_from(hep, e));
  COUNT("earray_copy", earray_copy(hap));
  COUNT("earray_from", earray_from(hap, e));
  COUNT("earray_store", earray_store(hap, e));
  COUNT("earray_side", if (earray_side(hap, e) != 1) reports += 100);
  COUNT("hvmember_from", hvmember_from(hvp, e));
  COUNT("hvmember_copy", hvmember_copy(hvp));
  COUNT("hvmember_disc", hvmember_disc(hvp));
  COUNT("nonnull_extra", ebasen_from(&den, e); ebasen_copy(&den);
        ebasen_move(&den, e); ebasen_store(&den, e); ebasen_call(&den, e);
        ebasen_addr(&den); ebasen_bind(&den); ebasen_disc(&den);
        emember_move(&he); emember_move_from(&he, e); earray_copy(&ha);
        earray_from(&ha, e); earray_store(&ha, e); earray_side(&ha, e);
        hvmember_from(&hv, e); hvmember_copy(&hv); hvmember_disc(&hv));
  return 0;
}

// ALL: earg 1
// ALL-NEXT: eret 1
// ALL-NEXT: ebrace 1
// ALL-NEXT: ecast 1
// ALL-NEXT: emove_from 1
// ALL-NEXT: ebase_from 1
// ALL-NEXT: evbase_from 1
// ALL-NEXT: tcopy 1
// ALL-NEXT: ebase_copy 1
// ALL-NEXT: ebase_arg 1
// ALL-NEXT: ebase_ret 1
// ALL-NEXT: ebase_store 1
// ALL-NEXT: ebase_call 1
// ALL-NEXT: ebase_move 1
// ALL-NEXT: ebase0_from 1
// ALL-NEXT: evbase_copy 1
// ALL-NEXT: emember_copy 1
// ALL-NEXT: emember_arg 1
// ALL-NEXT: emember_store 1
// ALL-NEXT: hdmember_base 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: ebasen_from 1
// ALL-NEXT: ebasen_copy 1
// ALL-NEXT: ebasen_move 1
// ALL-NEXT: ebasen_store 1
// ALL-NEXT: ebasen_call 1
// ALL-NEXT: ebasen_addr 1
// ALL-NEXT: ebasen_bind 1
// ALL-NEXT: ebasen_disc 1
// ALL-NEXT: emember_move 1
// ALL-NEXT: emember_move_from 1
// ALL-NEXT: earray_copy 1
// ALL-NEXT: earray_from 1
// ALL-NEXT: earray_store 1
// ALL-NEXT: earray_side 1
// ALL-NEXT: hvmember_from 1
// ALL-NEXT: hvmember_copy 1
// ALL-NEXT: hvmember_disc 1
// ALL-NEXT: nonnull_extra 0

// IND: earg 1
// IND-NEXT: eret 1
// IND-NEXT: ebrace 1
// IND-NEXT: ecast 1
// IND-NEXT: emove_from 1
// IND-NEXT: ebase_from 1
// IND-NEXT: evbase_from 0
// IND-NEXT: tcopy 1
// IND-NEXT: ebase_copy 1
// IND-NEXT: ebase_arg 1
// IND-NEXT: ebase_ret 1
// IND-NEXT: ebase_store 1
// IND-NEXT: ebase_call 1
// IND-NEXT: ebase_move 1
// IND-NEXT: ebase0_from 1
// IND-NEXT: evbase_copy 0
// IND-NEXT: emember_copy 1
// IND-NEXT: emember_arg 1
// IND-NEXT: emember_store 1
// IND-NEXT: hdmember_base 1
// IND-NEXT: nonnull 0
// IND-NEXT: ebasen_from 1
// IND-NEXT: ebasen_copy 1
// IND-NEXT: ebasen_move 1
// IND-NEXT: ebasen_store 1
// IND-NEXT: ebasen_call 1
// IND-NEXT: ebasen_addr 1
// IND-NEXT: ebasen_bind 1
// IND-NEXT: ebasen_disc 1
// IND-NEXT: emember_move 1
// IND-NEXT: emember_move_from 1
// IND-NEXT: earray_copy 1
// IND-NEXT: earray_from 1
// IND-NEXT: earray_store 1
// IND-NEXT: earray_side 1
// IND-NEXT: hvmember_from 0
// IND-NEXT: hvmember_copy 0
// IND-NEXT: hvmember_disc 0
// IND-NEXT: nonnull_extra 0

// ACC: earg 0
// ACC-NEXT: eret 0
// ACC-NEXT: ebrace 0
// ACC-NEXT: ecast 0
// ACC-NEXT: emove_from 0
// ACC-NEXT: ebase_from 0
// ACC-NEXT: evbase_from 1
// ACC-NEXT: tcopy 0
// ACC-NEXT: ebase_copy 0
// ACC-NEXT: ebase_arg 0
// ACC-NEXT: ebase_ret 0
// ACC-NEXT: ebase_store 0
// ACC-NEXT: ebase_call 0
// ACC-NEXT: ebase_move 0
// ACC-NEXT: ebase0_from 0
// ACC-NEXT: evbase_copy 1
// ACC-NEXT: emember_copy 0
// ACC-NEXT: emember_arg 0
// ACC-NEXT: emember_store 0
// ACC-NEXT: hdmember_base 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: ebasen_from 0
// ACC-NEXT: ebasen_copy 0
// ACC-NEXT: ebasen_move 0
// ACC-NEXT: ebasen_store 0
// ACC-NEXT: ebasen_call 0
// ACC-NEXT: ebasen_addr 0
// ACC-NEXT: ebasen_bind 0
// ACC-NEXT: ebasen_disc 0
// ACC-NEXT: emember_move 0
// ACC-NEXT: emember_move_from 0
// ACC-NEXT: earray_copy 0
// ACC-NEXT: earray_from 0
// ACC-NEXT: earray_store 0
// ACC-NEXT: earray_side 0
// ACC-NEXT: hvmember_from 1
// ACC-NEXT: hvmember_copy 1
// ACC-NEXT: hvmember_disc 1
// ACC-NEXT: nonnull_extra 0
