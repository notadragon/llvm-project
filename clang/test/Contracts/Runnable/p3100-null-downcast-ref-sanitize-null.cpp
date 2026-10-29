// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s --check-prefixes=CHECK,O0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s --check-prefixes=CHECK,O2

// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a downcast of `*p`
// to a reference, `p` null, is reported twice: by the P3100 check of the
// reference binding, member access or member call that uses it, and by the
// sanitizer's null check of the downcast itself (stock
// `TCK_DowncastReference`), which the P3100 check does not replace.
// CLANG-796, accepted under DECISIONS.md E120: dropping the downcast's
// sanitizer check only where the use's P3100 check is made would need the
// use's leaf passed to the downcast, which is emitted first, and keeping
// "at least once" where the use's leaf is ignored needs it too; stock
// -fsanitize=null alone reports these twice as well.  Forms: the downcast
// alone (bound to the returned reference), a read of a member through it,
// a call of a non-virtual member function through it (a third report at
// -O0, the callee's prologue check of `this`: CLANG-792, accepted,
// DECISIONS.md E114 (b)), for a polymorphic class and for one that is
// not.  Without -fsanitize=null each is reported once.  (A base at a
// nonzero offset: p3100-null-downcast-ref-offset.cpp, once.)
// (GCC: gnu_gcc reports each once where a null leaf applies, and the reads
// twice with the group ignored, accepted there under E120 too.)

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
  int v;
};
struct D : V {
  int d;
  void dn() {}
};
struct S { int s; };
struct DS : S { int t; };
NI D &dref(V *vp) { return static_cast<D &>(*vp); }
NI int drdc(V *vp) { return static_cast<D &>(*vp).d; }
NI DS &sref(S *sp) { return static_cast<DS &>(*sp); }
NI int srd(S *sp) { return static_cast<DS &>(*sp).t; }
NI void dcall(V *vp) { static_cast<D &>(*vp).dn(); }

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
  V *vp = reinterpret_cast<V *>(nv);
  S *sp = reinterpret_cast<S *>(nv);
  COUNT("dref", dref(vp));
  COUNT("drdc", drdc(vp));
  COUNT("sref", sref(sp));
  COUNT("srd", srd(sp));
  COUNT("dcall", dcall(vp));
  return 0;
}

// CHECK: dref 2
// CHECK-NEXT: drdc 2
// CHECK-NEXT: sref 2
// CHECK-NEXT: srd 2
// O0-NEXT: dcall 3
// O2-NEXT: dcall 2
