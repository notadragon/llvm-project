// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: more indirections through a null pointer whose result is not
// accessed (CLANG-753), each reported once under
// ub:expr.unary.dereference.nullptr.indirection: `&p[2]` (the subscripted
// pointer is checked, as for an accessed `p[i]`: CLANG-745), a discarded
// `p[0]`, conditional (either operand) or comma expression yielding `*p`,
// the address of a conditional or comma expression yielding `*p`, of `*p`
// cast to a reference (`static_cast<S &>`, `static_cast<const S &>`,
// `reinterpret_cast<int &>`) or converted to a base class (also at a nonzero
// offset, where p is checked before the conversion), the same discarded
// (also as `(S &)*dp` and `(void)static_cast<S &>(*sp)`),
// `std::addressof(*p)` and `__builtin_addressof(*p)` (also of a cast or a
// conditional), and the object expression of an enumerator, a static
// constexpr data member, a static member function and a pseudo-destructor
// named through `->` or `.` (also through a cast or a conditional), and of a
// trivial destructor called through `p` (CLANG-759).  `(&*p)->m` reports
// twice, the indirection of `&*p` and the access through it (the .access
// leaf, so once under .indirection alone).  The conversion of `*p` to a
// virtual base, which loads the base's offset through p, is the .access leaf
// (also discarded), as is that access of `(&*p)->m`: under .access alone
// only those report.  Not reported: `this`, `*&x`, an array, a reference
// (also cast), `typeid` of a polymorphic `*p` (which throws
// std::bad_typeid), `delete p`, an unevaluated operand.  A fault after the
// report is caught as SIGSEGV and resumed with siglongjmp, so one run
// counts every form.  The forms after `addr_vbase` are GCC's extra ones.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-unaccessed-indirection-edges{,-indirection,-access}.C,
// sharing p3100-null-unaccessed-indirection-edges.h; GCC reports every form
// once, as here, except `&p[2]`, for which it checks `p + 2`, GCC-794.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <memory>
#include <typeinfo>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

struct S {
  int m;
  enum { E = 5 };
  static constexpr int ck = 6;
  static int sf() { return 3; }
  int g();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};
struct P { virtual ~P() {} };
using I = int;

#define NI __attribute__((noinline))
NI int *addr2(int *p) { return &p[2]; }
NI void disc_sub(int *p) { p[0]; }
NI void disc_cond(int *p, int *q, bool b) { b ? *p : *q; }
NI void disc_comma(int *p) { (0, *p); }
NI void disc_comma_lhs(int *p) { (*p, 0); }
NI int *addr_cond(int *p, int *q, bool b) { return &(b ? *p : *q); }
NI int *addr_comma(int *p) { return &(0, *p); }
NI S *addr_cast(S *sp) { return &static_cast<S &>(*sp); }
NI const S *addr_ccast(S *sp) { return &static_cast<const S &>(*sp); }
NI int *addr_rcast(long *lp) { return &reinterpret_cast<int &>(*lp); }
NI A *addr_base(D *dp) { return &static_cast<A &>(*dp); }
NI S *addr_base_off(D *dp) { return &static_cast<S &>(*dp); }
NI void disc_cast(S *sp) { static_cast<S &>(*sp); }
NI void disc_base_off(D *dp) { static_cast<S &>(*dp); }
NI int *std_addressof(int *p) { return std::addressof(*p); }
NI int *builtin_addressof(int *p) { return __builtin_addressof(*p); }
NI int se(S *sp) { return sp->E; }
NI int sck(S *sp) { return sp->ck; }
NI void pdtor(int *p) { p->~I(); }
NI int amem(S *sp) { return (&*sp)->m; }
NI S *addr_vbase(V *vp) { return &static_cast<S &>(*vp); }
NI void disc_base(D *dp) { static_cast<A &>(*dp); }
NI void disc_ccast(S *sp) { static_cast<const S &>(*sp); }
NI void disc_rcast(long *lp) { reinterpret_cast<int &>(*lp); }
NI void disc_cbase(D *dp) { (S &)*dp; }
NI void disc_void_cast(S *sp) { (void)static_cast<S &>(*sp); }
NI void disc_vbase(V *vp) { static_cast<S &>(*vp); }
NI int se_star(S *sp) { return (*sp).E; }
NI int sck_cast(S *sp) { return static_cast<S &>(*sp).ck; }
NI int sf_cast(D *dp) { return static_cast<S &>(*dp).sf(); }
NI int se_cond(S *sp, S *sq, bool b) { return (b ? *sp : *sq).E; }
NI void pdtor_star(int *p) { (*p).~I(); }
NI void pdtor_cast(long *lp) { reinterpret_cast<int &>(*lp).~I(); }
NI void sdtor(S *sp) { sp->~S(); }
NI void sdtor_star(S *sp) { (*sp).~S(); }
NI S *builtin_addressof_cast(S *sp) {
  return __builtin_addressof(static_cast<S &>(*sp));
}
NI int *std_addressof_cond(int *p, int *q, bool b) {
  return std::addressof(b ? *p : *q);
}
// Controls.
int S::g() {
  (void)*this;
  S *q = &*this;
  static_cast<S &>(*this);
  return this->ck + (*this).E + (q == this);
}
int gx;
int garr[2];
NI int *addr_obj() { return &*&gx; }
NI int *addr_arr() { return &*garr; }
NI int *addr_ref(int &r) { return &r; }
NI void disc_ref(int &r) { r; }
NI void disc_ref_cast(int &r) { static_cast<int &>(r); }
NI bool tid(P *pp) {
  try {
    return typeid(*pp) == typeid(P);
  } catch (std::bad_typeid &) {
    return false;
  }
}
NI void del(S *sp) { delete sp; }
NI int unevaluated(int *p) { return sizeof(*p) + sizeof(decltype(&*p)); }

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
  int *p = vp;
  S *sp = reinterpret_cast<S *>(p);
  D *dp = reinterpret_cast<D *>(p);
  V *vbp = reinterpret_cast<V *>(p);
  P *pp = reinterpret_cast<P *>(p);
  long *lp = reinterpret_cast<long *>(p);
  int x = 0;
  S s{};
  D d{};
  V v;
  COUNT("addr2", addr2(p));
  COUNT("disc_sub", disc_sub(p));
  COUNT("disc_cond", disc_cond(&x, p, false));
  COUNT("disc_comma", disc_comma(p));
  COUNT("disc_comma_lhs", disc_comma_lhs(p));
  COUNT("addr_cond", addr_cond(&x, p, false));
  COUNT("addr_comma", addr_comma(p));
  COUNT("addr_cast", addr_cast(sp));
  COUNT("addr_ccast", addr_ccast(sp));
  COUNT("addr_rcast", addr_rcast(lp));
  COUNT("addr_base", addr_base(dp));
  COUNT("addr_base_off", addr_base_off(dp));
  COUNT("disc_cast", disc_cast(sp));
  COUNT("disc_base_off", disc_base_off(dp));
  COUNT("std_addressof", std_addressof(p));
  COUNT("builtin_addressof", builtin_addressof(p));
  COUNT("se", se(sp));
  COUNT("sck", sck(sp));
  COUNT("pdtor", pdtor(p));
  COUNT("amem", amem(sp));
  COUNT("addr_vbase", addr_vbase(vbp));
  COUNT("disc_base", disc_base(dp));
  COUNT("disc_ccast", disc_ccast(sp));
  COUNT("disc_rcast", disc_rcast(lp));
  COUNT("disc_cbase", disc_cbase(dp));
  COUNT("disc_void_cast", disc_void_cast(sp));
  COUNT("disc_vbase", disc_vbase(vbp));
  COUNT("se_star", se_star(sp));
  COUNT("sck_cast", sck_cast(sp));
  COUNT("sf_cast", sf_cast(dp));
  COUNT("se_cond", se_cond(&s, sp, false));
  COUNT("pdtor_star", pdtor_star(p));
  COUNT("pdtor_cast", pdtor_cast(lp));
  COUNT("sdtor", sdtor(sp));
  COUNT("sdtor_star", sdtor_star(sp));
  COUNT("builtin_addressof_cast", builtin_addressof_cast(sp));
  COUNT("std_addressof_cond", std_addressof_cond(&x, p, false));
  COUNT("nonnull", addr2(&x); disc_cond(&x, p, true); addr_cond(&x, p, true);
        addr_base_off(&d); disc_base_off(&d); se(&s); pdtor(&x); amem(&s);
        disc_vbase(&v); disc_cast(&s); sck_cast(&s);
        std_addressof_cond(&x, p, true));
  COUNT("controls", s.g(); addr_obj(); addr_arr(); addr_ref(x); disc_ref(x);
        disc_ref_cast(x); tid(pp); del(sp); unevaluated(p));
  return 0;
}

// ALL: addr2 1
// ALL-NEXT: disc_sub 1
// ALL-NEXT: disc_cond 1
// ALL-NEXT: disc_comma 1
// ALL-NEXT: disc_comma_lhs 1
// ALL-NEXT: addr_cond 1
// ALL-NEXT: addr_comma 1
// ALL-NEXT: addr_cast 1
// ALL-NEXT: addr_ccast 1
// ALL-NEXT: addr_rcast 1
// ALL-NEXT: addr_base 1
// ALL-NEXT: addr_base_off 1
// ALL-NEXT: disc_cast 1
// ALL-NEXT: disc_base_off 1
// ALL-NEXT: std_addressof 1
// ALL-NEXT: builtin_addressof 1
// ALL-NEXT: se 1
// ALL-NEXT: sck 1
// ALL-NEXT: pdtor 1
// ALL-NEXT: amem 2
// ALL-NEXT: addr_vbase 1
// ALL-NEXT: disc_base 1
// ALL-NEXT: disc_ccast 1
// ALL-NEXT: disc_rcast 1
// ALL-NEXT: disc_cbase 1
// ALL-NEXT: disc_void_cast 1
// ALL-NEXT: disc_vbase 1
// ALL-NEXT: se_star 1
// ALL-NEXT: sck_cast 1
// ALL-NEXT: sf_cast 1
// ALL-NEXT: se_cond 1
// ALL-NEXT: pdtor_star 1
// ALL-NEXT: pdtor_cast 1
// ALL-NEXT: sdtor 1
// ALL-NEXT: sdtor_star 1
// ALL-NEXT: builtin_addressof_cast 1
// ALL-NEXT: std_addressof_cond 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: controls 0

// IND: addr2 1
// IND-NEXT: disc_sub 1
// IND-NEXT: disc_cond 1
// IND-NEXT: disc_comma 1
// IND-NEXT: disc_comma_lhs 1
// IND-NEXT: addr_cond 1
// IND-NEXT: addr_comma 1
// IND-NEXT: addr_cast 1
// IND-NEXT: addr_ccast 1
// IND-NEXT: addr_rcast 1
// IND-NEXT: addr_base 1
// IND-NEXT: addr_base_off 1
// IND-NEXT: disc_cast 1
// IND-NEXT: disc_base_off 1
// IND-NEXT: std_addressof 1
// IND-NEXT: builtin_addressof 1
// IND-NEXT: se 1
// IND-NEXT: sck 1
// IND-NEXT: pdtor 1
// IND-NEXT: amem 1
// IND-NEXT: addr_vbase 0
// IND-NEXT: disc_base 1
// IND-NEXT: disc_ccast 1
// IND-NEXT: disc_rcast 1
// IND-NEXT: disc_cbase 1
// IND-NEXT: disc_void_cast 1
// IND-NEXT: disc_vbase 0
// IND-NEXT: se_star 1
// IND-NEXT: sck_cast 1
// IND-NEXT: sf_cast 1
// IND-NEXT: se_cond 1
// IND-NEXT: pdtor_star 1
// IND-NEXT: pdtor_cast 1
// IND-NEXT: sdtor 1
// IND-NEXT: sdtor_star 1
// IND-NEXT: builtin_addressof_cast 1
// IND-NEXT: std_addressof_cond 1
// IND-NEXT: nonnull 0
// IND-NEXT: controls 0

// ACC: addr2 0
// ACC-NEXT: disc_sub 0
// ACC-NEXT: disc_cond 0
// ACC-NEXT: disc_comma 0
// ACC-NEXT: disc_comma_lhs 0
// ACC-NEXT: addr_cond 0
// ACC-NEXT: addr_comma 0
// ACC-NEXT: addr_cast 0
// ACC-NEXT: addr_ccast 0
// ACC-NEXT: addr_rcast 0
// ACC-NEXT: addr_base 0
// ACC-NEXT: addr_base_off 0
// ACC-NEXT: disc_cast 0
// ACC-NEXT: disc_base_off 0
// ACC-NEXT: std_addressof 0
// ACC-NEXT: builtin_addressof 0
// ACC-NEXT: se 0
// ACC-NEXT: sck 0
// ACC-NEXT: pdtor 0
// ACC-NEXT: amem 1
// ACC-NEXT: addr_vbase 1
// ACC-NEXT: disc_base 0
// ACC-NEXT: disc_ccast 0
// ACC-NEXT: disc_rcast 0
// ACC-NEXT: disc_cbase 0
// ACC-NEXT: disc_void_cast 0
// ACC-NEXT: disc_vbase 1
// ACC-NEXT: se_star 0
// ACC-NEXT: sck_cast 0
// ACC-NEXT: sf_cast 0
// ACC-NEXT: se_cond 0
// ACC-NEXT: pdtor_star 0
// ACC-NEXT: pdtor_cast 0
// ACC-NEXT: sdtor 0
// ACC-NEXT: sdtor_star 0
// ACC-NEXT: builtin_addressof_cast 0
// ACC-NEXT: std_addressof_cond 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: controls 0
