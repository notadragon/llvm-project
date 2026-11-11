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

// P3100: a call through a virtual pointer to member function reads the
// vtable pointer through the object pointer: with that pointer null it is
// reported once, under .access, as a virtual call is (DECISIONS.md E102
// (a)) -- also on the element at a constant subscript, `(vp[1].*p)()`, or
// at a constant offset, `((vp + 1)->*p)()`, and with a nonzero `this`
// adjustment (a virtual member function of a second polymorphic base).  A
// non-virtual pointer to member function reads nothing: the same calls
// are reported once under .indirection.  Which it is only the pointer's
// value says, so the check is made after the pointer to member function
// is evaluated.  Through `this` in a member function called through a
// null pointer, nothing more is reported than the call (DECISIONS.md E106
// (a)).  A non-null pointer is not reported.
// (CLANG-781: the virtual forms were .indirection.)
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-virtual-pmf*.C,
// whose header p3100-null-virtual-pmf.h has the same forms in this order.

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
struct V {
  virtual void h() {}
  void nv() {}
  int v;
  NI void self(void (V::*p)()) { (this->*p)(); }
};
struct W1 { virtual void w1() {} int q; };
struct W2 { virtual void w2() {} void nw2() {} int r; };
struct MW : W1, W2 { int z; };

typedef void (V::*VPM)();
typedef void (MW::*MPM)();
NI void call(V *vp, VPM p) { (vp->*p)(); }
NI void callsub(V *vp, VPM p) { (vp[1].*p)(); }
NI void callarith(V *vp, VPM p) { ((vp + 1)->*p)(); }
NI void calladj(MW *mp, MPM p) { (mp->*p)(); }
NI void calladjsub(MW *mp, MPM p) { (mp[1].*p)(); }

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
  MW *mp = reinterpret_cast<MW *>(nv);
  V va[2];
  MW ma[2];
  COUNT("virt", call(vp, &V::h));
  COUNT("virtsub", callsub(vp, &V::h));
  COUNT("virtarith", callarith(vp, &V::h));
  COUNT("virtadj", calladj(mp, &MW::w2));
  COUNT("virtadjsub", calladjsub(mp, &MW::w2));
  COUNT("nonvirt", call(vp, &V::nv));
  COUNT("nonvirtsub", callsub(vp, &V::nv));
  COUNT("nonvirtarith", callarith(vp, &V::nv));
  COUNT("nonvirtadj", calladj(mp, &MW::nw2));
  COUNT("nonvirtadjsub", calladjsub(mp, &MW::nw2));
  COUNT("self", vp->self(&V::h));
  COUNT("nonnull", call(va, &V::h); callsub(va, &V::h);
        callarith(va, &V::h); calladj(ma, &MW::w2);
        calladjsub(ma, &MW::w2); call(va, &V::nv); callsub(va, &V::nv);
        callarith(va, &V::nv); calladj(ma, &MW::nw2);
        calladjsub(ma, &MW::nw2); va[0].self(&V::h));
  return 0;
}

// ALL: virt 1
// ALL-NEXT: virtsub 1
// ALL-NEXT: virtarith 1
// ALL-NEXT: virtadj 1
// ALL-NEXT: virtadjsub 1
// ALL-NEXT: nonvirt 1
// ALL-NEXT: nonvirtsub 1
// ALL-NEXT: nonvirtarith 1
// ALL-NEXT: nonvirtadj 1
// ALL-NEXT: nonvirtadjsub 1
// ALL-NEXT: self 1
// ALL-NEXT: nonnull 0

// IND: virt 0
// IND-NEXT: virtsub 0
// IND-NEXT: virtarith 0
// IND-NEXT: virtadj 0
// IND-NEXT: virtadjsub 0
// IND-NEXT: nonvirt 1
// IND-NEXT: nonvirtsub 1
// IND-NEXT: nonvirtarith 1
// IND-NEXT: nonvirtadj 1
// IND-NEXT: nonvirtadjsub 1
// IND-NEXT: self 1
// IND-NEXT: nonnull 0

// ACC: virt 1
// ACC-NEXT: virtsub 1
// ACC-NEXT: virtarith 1
// ACC-NEXT: virtadj 1
// ACC-NEXT: virtadjsub 1
// ACC-NEXT: nonvirt 0
// ACC-NEXT: nonvirtsub 0
// ACC-NEXT: nonvirtarith 0
// ACC-NEXT: nonvirtadj 0
// ACC-NEXT: nonvirtadjsub 0
// ACC-NEXT: self 0
// ACC-NEXT: nonnull 0
