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

// P3100: a member function called through a null pointer is reported once,
// under .indirection, where it is called; a read or write of a member
// through `this` inside it is not checked (DECISIONS.md E100 (a)), at any
// optimization level: by name, through `this->` or `(*this).`, an element
// of an array member, a member of a member, in a lambda capturing `this`
// (by `[this]`, `[&]`, or the copy `[*this]`), and a copy of `*this`, which
// reads through `this` too.  A read through a pointer parameter of a static
// member function is checked as any other (.access).  A non-null pointer is
// not reported.  Then Clang's extra forms: a store through `(*this).`, in
// a lambda capturing `this` or by reference, `(*this).a` and a copy of
// `*this` in a lambda capturing `this`, an assignment to and from `*this`,
// an element of an array member and a member of a member through
// `(*this).`, `this->*pm` and `(*this).*pm`, `*this` moved (by a cast and by
// `std::move`) and passed by value, `this[0].a`, and, in a class with a
// base and a virtual base, a member of each read by name, through
// `(*this).` and through a cast of `*this` to the base, and the base
// copied.  Then GCC's: a store through `this->*pm` and `(*this).*pm`, and
// a read through each in a lambda capturing `this`; and a read or write
// through a non-zero subscript of `this` (DECISIONS.md E108 (a)):
// `this[1].a` read and stored to, `this[i].a` read and stored to, an
// element of an array member, in a lambda capturing `this`, and a copy of
// `this[1]`.  The indirections through `this` that read or write nothing
// (DECISIONS.md E106 (a)) are Runnable/p3100-null-this-indirection.cpp.
// (CLANG-784: a read or write spelled with an explicit indirection through
// `this` -- `(*this).`, `this[0]`, `->*`, a copy of `*this` -- was checked
// again inside the member function at -O0, under .access.)
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-this-read*.C,
// whose header p3100-null-this-read.h has the same forms in this order.

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

#define NI __attribute__((noinline))
struct T;
NI int byvalue(T t);
struct T {
  int a;
  int arr[2];
  struct { int x; } s;
  NI int g();
  NI void st();
  NI int ex() { return this->a; }
  NI int dr() { return (*this).a; }
  NI int ar() { return arr[1]; }
  NI int sx() { return s.x; }
  NI int lam() { return [this] { return a; }(); }
  NI int lamcopy() { return [*this] { return a; }(); }
  NI int lamref() { return [&] { return a; }(); }
  NI int copy() { T t2 = *this; return t2.a; }
  NI static int stat(T *tp) { return tp->a; }
  // Clang's.
  NI void dst() { (*this).a = 1; }
  NI void lamst() { [this] { a = 1; }(); }
  NI void lamrefst() { [&] { a = 1; }(); }
  NI void lamdr() { [this] { (*this).a = 1; }(); }
  NI int lamthiscopy() { return [this] { T t2 = *this; return t2.a; }(); }
  NI void assignto(T &o) { o = *this; }
  NI void assignfrom(const T &o) { *this = o; }
  NI int arrdr() { return (*this).arr[1]; }
  NI int sdr() { return (*this).s.x; }
  NI int pmarrow(int T::*p) { return this->*p; }
  NI int pmdot(int T::*p) { return (*this).*p; }
  NI int movecopy() { T t2 = static_cast<T &&>(*this); return t2.a; }
  NI int stdmove() { T t2 = std::move(*this); return t2.a; }
  NI int byval() { return byvalue(*this); }
  NI int sub0() { return this[0].a; }
  // GCC's.
  NI void pmstore(int T::*p) { this->*p = 1; }
  NI void pmstored(int T::*p) { (*this).*p = 1; }
  NI int lampm(int T::*p) { return [this, p] { return this->*p; }(); }
  NI int lampmd(int T::*p) { return [this, p] { return (*this).*p; }(); }
  // GCC's: a read or write through a non-zero subscript of `this`.
  NI int sub1() { return this[1].a; }
  NI void sub1st() { this[1].a = 1; }
  NI int subi(int i) { return this[i].a; }
  NI void subist(int i) { this[i].a = 1; }
  NI int sub1arr() { return this[1].arr[1]; }
  NI int lamsub1() { return [this] { return this[1].a; }(); }
  NI int sub1copy() { T t2 = this[1]; return t2.a; }
};
int T::g() { return a; }
void T::st() { a = 1; }
int byvalue(T t) { return t.a; }

struct B { int b; };
struct VB { int vb; };
struct U : B, virtual VB {
  int u;
  NI int base() { return b; }
  NI int dbase() { return (*this).b; }
  NI int cbase() { return static_cast<B &>(*this).b; }
  NI int bcopy() { B b2 = *this; return b2.b; }
  NI int vbase() { return vb; }
  NI int dvbase() { return (*this).vb; }
  NI int cvbase() { return static_cast<VB &>(*this).vb; }
};

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
  T *tp = reinterpret_cast<T *>(nv);
  U *up = reinterpret_cast<U *>(nv);
  T t{}, t2{}, ta[2]{};
  U u;
  COUNT("g", tp->g());
  COUNT("st", tp->st());
  COUNT("ex", tp->ex());
  COUNT("dr", tp->dr());
  COUNT("ar", tp->ar());
  COUNT("sx", tp->sx());
  COUNT("lam", tp->lam());
  COUNT("lamcopy", tp->lamcopy());
  COUNT("lamref", tp->lamref());
  COUNT("copy", tp->copy());
  COUNT("stat", T::stat(tp));
  COUNT("nonnull", t.g(); t.st(); t.ex(); t.dr(); t.ar(); t.sx();
        t.lam(); t.lamcopy(); t.lamref(); t.copy(); T::stat(&t));
  COUNT("dst", tp->dst());
  COUNT("lamst", tp->lamst());
  COUNT("lamrefst", tp->lamrefst());
  COUNT("lamdr", tp->lamdr());
  COUNT("lamthiscopy", tp->lamthiscopy());
  COUNT("assignto", tp->assignto(t2));
  COUNT("assignfrom", tp->assignfrom(t2));
  COUNT("arrdr", tp->arrdr());
  COUNT("sdr", tp->sdr());
  COUNT("pmarrow", tp->pmarrow(&T::a));
  COUNT("pmdot", tp->pmdot(&T::a));
  COUNT("movecopy", tp->movecopy());
  COUNT("stdmove", tp->stdmove());
  COUNT("byval", tp->byval());
  COUNT("sub0", tp->sub0());
  COUNT("base", up->base());
  COUNT("dbase", up->dbase());
  COUNT("cbase", up->cbase());
  COUNT("bcopy", up->bcopy());
  COUNT("vbase", up->vbase());
  COUNT("dvbase", up->dvbase());
  COUNT("cvbase", up->cvbase());
  COUNT("nonnull_extra", t.dst(); t.lamst(); t.lamrefst(); t.lamdr();
        t.lamthiscopy(); t.assignto(t2); t.assignfrom(t2); t.arrdr();
        t.sdr(); t.pmarrow(&T::a); t.pmdot(&T::a); t.movecopy();
        t.stdmove(); t.byval(); t.sub0(); u.base(); u.dbase(); u.cbase();
        u.bcopy(); u.vbase(); u.dvbase(); u.cvbase());
  COUNT("pmstore", tp->pmstore(&T::a));
  COUNT("pmstored", tp->pmstored(&T::a));
  COUNT("lampm", tp->lampm(&T::a));
  COUNT("lampmd", tp->lampmd(&T::a));
  COUNT("nonnull_gcc", t.pmstore(&T::a); t.pmstored(&T::a);
        t.lampm(&T::a); t.lampmd(&T::a));
  COUNT("sub1", tp->sub1());
  COUNT("sub1st", tp->sub1st());
  COUNT("subi", tp->subi(1));
  COUNT("subist", tp->subist(1));
  COUNT("sub1arr", tp->sub1arr());
  COUNT("lamsub1", tp->lamsub1());
  COUNT("sub1copy", tp->sub1copy());
  COUNT("nonnull_sub", ta[0].sub1(); ta[0].sub1st(); ta[0].subi(1);
        ta[0].subist(1); ta[0].sub1arr(); ta[0].lamsub1();
        ta[0].sub1copy());
  return 0;
}

// ALL: g 1
// ALL-NEXT: st 1
// ALL-NEXT: ex 1
// ALL-NEXT: dr 1
// ALL-NEXT: ar 1
// ALL-NEXT: sx 1
// ALL-NEXT: lam 1
// ALL-NEXT: lamcopy 1
// ALL-NEXT: lamref 1
// ALL-NEXT: copy 1
// ALL-NEXT: stat 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: dst 1
// ALL-NEXT: lamst 1
// ALL-NEXT: lamrefst 1
// ALL-NEXT: lamdr 1
// ALL-NEXT: lamthiscopy 1
// ALL-NEXT: assignto 1
// ALL-NEXT: assignfrom 1
// ALL-NEXT: arrdr 1
// ALL-NEXT: sdr 1
// ALL-NEXT: pmarrow 1
// ALL-NEXT: pmdot 1
// ALL-NEXT: movecopy 1
// ALL-NEXT: stdmove 1
// ALL-NEXT: byval 1
// ALL-NEXT: sub0 1
// ALL-NEXT: base 1
// ALL-NEXT: dbase 1
// ALL-NEXT: cbase 1
// ALL-NEXT: bcopy 1
// ALL-NEXT: vbase 1
// ALL-NEXT: dvbase 1
// ALL-NEXT: cvbase 1
// ALL-NEXT: nonnull_extra 0
// ALL-NEXT: pmstore 1
// ALL-NEXT: pmstored 1
// ALL-NEXT: lampm 1
// ALL-NEXT: lampmd 1
// ALL-NEXT: nonnull_gcc 0
// ALL-NEXT: sub1 1
// ALL-NEXT: sub1st 1
// ALL-NEXT: subi 1
// ALL-NEXT: subist 1
// ALL-NEXT: sub1arr 1
// ALL-NEXT: lamsub1 1
// ALL-NEXT: sub1copy 1
// ALL-NEXT: nonnull_sub 0

// IND: g 1
// IND-NEXT: st 1
// IND-NEXT: ex 1
// IND-NEXT: dr 1
// IND-NEXT: ar 1
// IND-NEXT: sx 1
// IND-NEXT: lam 1
// IND-NEXT: lamcopy 1
// IND-NEXT: lamref 1
// IND-NEXT: copy 1
// IND-NEXT: stat 0
// IND-NEXT: nonnull 0
// IND-NEXT: dst 1
// IND-NEXT: lamst 1
// IND-NEXT: lamrefst 1
// IND-NEXT: lamdr 1
// IND-NEXT: lamthiscopy 1
// IND-NEXT: assignto 1
// IND-NEXT: assignfrom 1
// IND-NEXT: arrdr 1
// IND-NEXT: sdr 1
// IND-NEXT: pmarrow 1
// IND-NEXT: pmdot 1
// IND-NEXT: movecopy 1
// IND-NEXT: stdmove 1
// IND-NEXT: byval 1
// IND-NEXT: sub0 1
// IND-NEXT: base 1
// IND-NEXT: dbase 1
// IND-NEXT: cbase 1
// IND-NEXT: bcopy 1
// IND-NEXT: vbase 1
// IND-NEXT: dvbase 1
// IND-NEXT: cvbase 1
// IND-NEXT: nonnull_extra 0
// IND-NEXT: pmstore 1
// IND-NEXT: pmstored 1
// IND-NEXT: lampm 1
// IND-NEXT: lampmd 1
// IND-NEXT: nonnull_gcc 0
// IND-NEXT: sub1 1
// IND-NEXT: sub1st 1
// IND-NEXT: subi 1
// IND-NEXT: subist 1
// IND-NEXT: sub1arr 1
// IND-NEXT: lamsub1 1
// IND-NEXT: sub1copy 1
// IND-NEXT: nonnull_sub 0

// ACC: g 0
// ACC-NEXT: st 0
// ACC-NEXT: ex 0
// ACC-NEXT: dr 0
// ACC-NEXT: ar 0
// ACC-NEXT: sx 0
// ACC-NEXT: lam 0
// ACC-NEXT: lamcopy 0
// ACC-NEXT: lamref 0
// ACC-NEXT: copy 0
// ACC-NEXT: stat 1
// ACC-NEXT: nonnull 0
// ACC-NEXT: dst 0
// ACC-NEXT: lamst 0
// ACC-NEXT: lamrefst 0
// ACC-NEXT: lamdr 0
// ACC-NEXT: lamthiscopy 0
// ACC-NEXT: assignto 0
// ACC-NEXT: assignfrom 0
// ACC-NEXT: arrdr 0
// ACC-NEXT: sdr 0
// ACC-NEXT: pmarrow 0
// ACC-NEXT: pmdot 0
// ACC-NEXT: movecopy 0
// ACC-NEXT: stdmove 0
// ACC-NEXT: byval 0
// ACC-NEXT: sub0 0
// ACC-NEXT: base 0
// ACC-NEXT: dbase 0
// ACC-NEXT: cbase 0
// ACC-NEXT: bcopy 0
// ACC-NEXT: vbase 0
// ACC-NEXT: dvbase 0
// ACC-NEXT: cvbase 0
// ACC-NEXT: nonnull_extra 0
// ACC-NEXT: pmstore 0
// ACC-NEXT: pmstored 0
// ACC-NEXT: lampm 0
// ACC-NEXT: lampmd 0
// ACC-NEXT: nonnull_gcc 0
// ACC-NEXT: sub1 0
// ACC-NEXT: sub1st 0
// ACC-NEXT: subi 0
// ACC-NEXT: subist 0
// ACC-NEXT: sub1arr 0
// ACC-NEXT: lamsub1 0
// ACC-NEXT: sub1copy 0
// ACC-NEXT: nonnull_sub 0
