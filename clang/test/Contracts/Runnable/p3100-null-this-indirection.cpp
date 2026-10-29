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
// under .indirection, where it is called, at every optimization level; an
// indirection through `this` inside it that reads or writes nothing is not
// checked (DECISIONS.md E106 (a), extending E100 (a) to every use of
// `this`): a reference bound to `*this` (also const, to a base, a
// reference argument), to a member through `this->` or `(*this).`, the
// address of a member through either, of `this->*pm` and `(*this).*pm`,
// and of `this[0].a`, a member function called through `this->`, by name
// or through `(*this).`, and in a lambda capturing `this` a member
// function called, a member's address and a reference bound to `*this`.
// Nor through a non-zero subscript of `this` (DECISIONS.md E108 (a)): the
// address of `this[1].a`, `this[i].a` and `this[1]`, a reference bound to
// `this[1]`, a member function called on it, also in a lambda.  Nor is the
// parameter of a non-trivial copy constructor bound to `*this` (a copy of
// `*this`, the `[*this]` capture).  `&*this`, a discarded `*this` and a
// static member named through it (also in a lambda) indirect through
// nothing checked.  A non-null pointer is not reported.
// (CLANG-785: at -O0 a reference bound to `*this`, `&(*this).a`,
// `int &r = (*this).a`, `&(this->*pm)`, `&((*this).*pm)`, `&this[0].a`,
// the lambda's binding and the copy constructor's parameter were reported
// a second time, under .indirection, inside the member function.)  The
// reads and writes through `this` are Runnable/p3100-null-this-read.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-this-indirection*.C,
// whose header p3100-null-this-indirection.h has the same forms in this
// order.

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
struct B0 { int b; };
struct T;
NI void take(T &) {}
struct T : B0 {
  int a;
  static int s;
  NI int g() { return 0; }
  // References bound to `*this` and to a member, member addresses.
  NI int bind() { T &r = *this; (void)r; return 0; }
  NI int cbind() { const T &r = *this; (void)r; return 0; }
  NI int bbind() { B0 &r = *this; (void)r; return 0; }
  NI int targ() { take(*this); return 0; }
  NI int mbind() { int &r = this->a; (void)r; return 0; }
  NI int *maddr() { return &this->a; }
  NI int *daddr() { return &(*this).a; }
  NI int dmbind() { int &r = (*this).a; (void)r; return 0; }
  NI int *pmaddr(int T::*p) { return &(this->*p); }
  NI int *pmdaddr(int T::*p) { return &((*this).*p); }
  NI int *sub0addr() { return &this[0].a; }
  // Member functions called on `*this`.
  NI int tcall() { return this->g(); }
  NI int icall() { return g(); }
  NI int dcall() { return (*this).g(); }
  // In a lambda capturing `this`.
  NI int lamcall() { return [this] { return g(); }(); }
  NI int *lamaddr() { return [this] { return &a; }(); }
  NI int lambind() { return [this] { T &r = *this; (void)r; return 0; }(); }
  // Through a non-zero subscript of `this` (DECISIONS.md E108 (a)).
  NI int *sub1addr() { return &this[1].a; }
  NI int *subiaddr(int i) { return &this[i].a; }
  NI T *sub1p() { return &this[1]; }
  NI int sub1bind() { T &r = this[1]; (void)r; return 0; }
  NI int sub1call() { return this[1].g(); }
  NI int lamsub1call() { return [this] { return this[1].g(); }(); }
  // Neither the address `&*this`, a discarded `*this` nor a static member
  // named through it indirects through anything checked.
  NI T *addr() { return &*this; }
  NI void disc() { *this; }
  NI int sstat() { return (*this).s; }
  NI T *lamthisaddr() { return [this] { return &*this; }(); }
  NI void lamdisc() { [this] { *this; }(); }
};
int T::s;

// A class with a non-trivial copy constructor: its reference parameter
// is bound to `*this`.
struct N {
  int n;
  N() : n(0) {}
  N(const N &o) : n(o.n) {}
  NI int copy();
  NI int lamcopy();
};
int N::copy() { N n2 = *this; return n2.n; }
int N::lamcopy() { return [*this] { return n; }(); }

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
  N *np = reinterpret_cast<N *>(nv);
  T t[2]{};
  N n;
  COUNT("g", tp->g());
  COUNT("bind", tp->bind());
  COUNT("cbind", tp->cbind());
  COUNT("bbind", tp->bbind());
  COUNT("targ", tp->targ());
  COUNT("mbind", tp->mbind());
  COUNT("maddr", tp->maddr());
  COUNT("daddr", tp->daddr());
  COUNT("dmbind", tp->dmbind());
  COUNT("pmaddr", tp->pmaddr(&T::a));
  COUNT("pmdaddr", tp->pmdaddr(&T::a));
  COUNT("sub0addr", tp->sub0addr());
  COUNT("tcall", tp->tcall());
  COUNT("icall", tp->icall());
  COUNT("dcall", tp->dcall());
  COUNT("lamcall", tp->lamcall());
  COUNT("lamaddr", tp->lamaddr());
  COUNT("lambind", tp->lambind());
  COUNT("sub1addr", tp->sub1addr());
  COUNT("subiaddr", tp->subiaddr(1));
  COUNT("sub1p", tp->sub1p());
  COUNT("sub1bind", tp->sub1bind());
  COUNT("sub1call", tp->sub1call());
  COUNT("lamsub1call", tp->lamsub1call());
  COUNT("addr", tp->addr());
  COUNT("disc", tp->disc());
  COUNT("sstat", tp->sstat());
  COUNT("lamthisaddr", tp->lamthisaddr());
  COUNT("lamdisc", tp->lamdisc());
  COUNT("ncopy", np->copy());
  COUNT("nlamcopy", np->lamcopy());
  COUNT("nonnull", t[0].g(); t[0].bind(); t[0].cbind(); t[0].bbind();
        t[0].targ(); t[0].mbind(); t[0].maddr(); t[0].daddr();
        t[0].dmbind(); t[0].pmaddr(&T::a); t[0].pmdaddr(&T::a);
        t[0].sub0addr(); t[0].tcall(); t[0].icall(); t[0].dcall();
        t[0].lamcall(); t[0].lamaddr(); t[0].lambind(); t[0].sub1addr();
        t[0].subiaddr(1); t[0].sub1p(); t[0].sub1bind(); t[0].sub1call();
        t[0].lamsub1call(); t[0].addr(); t[0].disc(); t[0].sstat();
        t[0].lamthisaddr(); t[0].lamdisc(); n.copy(); n.lamcopy());
  return 0;
}

// ALL: g 1
// ALL-NEXT: bind 1
// ALL-NEXT: cbind 1
// ALL-NEXT: bbind 1
// ALL-NEXT: targ 1
// ALL-NEXT: mbind 1
// ALL-NEXT: maddr 1
// ALL-NEXT: daddr 1
// ALL-NEXT: dmbind 1
// ALL-NEXT: pmaddr 1
// ALL-NEXT: pmdaddr 1
// ALL-NEXT: sub0addr 1
// ALL-NEXT: tcall 1
// ALL-NEXT: icall 1
// ALL-NEXT: dcall 1
// ALL-NEXT: lamcall 1
// ALL-NEXT: lamaddr 1
// ALL-NEXT: lambind 1
// ALL-NEXT: sub1addr 1
// ALL-NEXT: subiaddr 1
// ALL-NEXT: sub1p 1
// ALL-NEXT: sub1bind 1
// ALL-NEXT: sub1call 1
// ALL-NEXT: lamsub1call 1
// ALL-NEXT: addr 1
// ALL-NEXT: disc 1
// ALL-NEXT: sstat 1
// ALL-NEXT: lamthisaddr 1
// ALL-NEXT: lamdisc 1
// ALL-NEXT: ncopy 1
// ALL-NEXT: nlamcopy 1
// ALL-NEXT: nonnull 0

// IND: g 1
// IND-NEXT: bind 1
// IND-NEXT: cbind 1
// IND-NEXT: bbind 1
// IND-NEXT: targ 1
// IND-NEXT: mbind 1
// IND-NEXT: maddr 1
// IND-NEXT: daddr 1
// IND-NEXT: dmbind 1
// IND-NEXT: pmaddr 1
// IND-NEXT: pmdaddr 1
// IND-NEXT: sub0addr 1
// IND-NEXT: tcall 1
// IND-NEXT: icall 1
// IND-NEXT: dcall 1
// IND-NEXT: lamcall 1
// IND-NEXT: lamaddr 1
// IND-NEXT: lambind 1
// IND-NEXT: sub1addr 1
// IND-NEXT: subiaddr 1
// IND-NEXT: sub1p 1
// IND-NEXT: sub1bind 1
// IND-NEXT: sub1call 1
// IND-NEXT: lamsub1call 1
// IND-NEXT: addr 1
// IND-NEXT: disc 1
// IND-NEXT: sstat 1
// IND-NEXT: lamthisaddr 1
// IND-NEXT: lamdisc 1
// IND-NEXT: ncopy 1
// IND-NEXT: nlamcopy 1
// IND-NEXT: nonnull 0

// ACC: g 0
// ACC-NEXT: bind 0
// ACC-NEXT: cbind 0
// ACC-NEXT: bbind 0
// ACC-NEXT: targ 0
// ACC-NEXT: mbind 0
// ACC-NEXT: maddr 0
// ACC-NEXT: daddr 0
// ACC-NEXT: dmbind 0
// ACC-NEXT: pmaddr 0
// ACC-NEXT: pmdaddr 0
// ACC-NEXT: sub0addr 0
// ACC-NEXT: tcall 0
// ACC-NEXT: icall 0
// ACC-NEXT: dcall 0
// ACC-NEXT: lamcall 0
// ACC-NEXT: lamaddr 0
// ACC-NEXT: lambind 0
// ACC-NEXT: sub1addr 0
// ACC-NEXT: subiaddr 0
// ACC-NEXT: sub1p 0
// ACC-NEXT: sub1bind 0
// ACC-NEXT: sub1call 0
// ACC-NEXT: lamsub1call 0
// ACC-NEXT: addr 0
// ACC-NEXT: disc 0
// ACC-NEXT: sstat 0
// ACC-NEXT: lamthisaddr 0
// ACC-NEXT: lamdisc 0
// ACC-NEXT: ncopy 0
// ACC-NEXT: nlamcopy 0
// ACC-NEXT: nonnull 0
