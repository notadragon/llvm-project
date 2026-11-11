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

// P3100: a dynamic_cast to a reference type of the object a null pointer
// points to (`dynamic_cast<W &>(*vp)`, to a const reference, of an element
// `vp[1]`) reads the vtable pointer through the pointer: it is reported
// once, under .access (DECISIONS.md E102 (a)).  One of a member through a
// null pointer is the member access's, reported once, under .access too;
// one of a reference parameter is not checked again (the null reference is
// reported once, where it is bound, under .indirection), nor one of an
// object.  Then Clang's: one at an offset, `*(vp + 1)`, and of `*xp`
// converted to a polymorphic base at a nonzero offset, each once under
// .access.
// (CLANG-780: none of these was checked.  CLANG-781: they were
// .indirection.)
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-null-dynamic-cast-reference*.C, whose header has the forms up to
// `nonnull` in this order.

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

struct V { virtual void h() {} int v; };
struct W : V {};
struct H { int x; V m; };
struct P { int p; };
struct X : P, V {};

#define NI __attribute__((noinline))
NI void star(V *vp) {
  try { (void)dynamic_cast<W &>(*vp); } catch (...) {}
}
NI void cstar(V *vp) {
  try { (void)dynamic_cast<const W &>(*vp); } catch (...) {}
}
NI void sub(V *vp) {
  try { (void)dynamic_cast<W &>(vp[1]); } catch (...) {}
}
NI void member(H *hp) {
  try { (void)dynamic_cast<W &>(hp->m); } catch (...) {}
}
NI void refparam(V &r) {
  try { (void)dynamic_cast<W &>(r); } catch (...) {}
}
// Clang's.
NI void arith(V *vp) {
  try { (void)dynamic_cast<W &>(*(vp + 1)); } catch (...) {}
}
NI void base(X *xp) {
  try { (void)dynamic_cast<W &>(static_cast<V &>(*xp)); } catch (...) {}
}

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
  H *hp = reinterpret_cast<H *>(nv);
  W w;
  V va[2];
  H h{};
  X x;
  COUNT("star", star(vp));
  COUNT("cstar", cstar(vp));
  COUNT("sub", sub(vp));
  COUNT("member", member(hp));
  COUNT("refparam", refparam(*vp));
  COUNT("nonnull", star(&w); cstar(&w); sub(va); member(&h); refparam(w));
  COUNT("arith", arith(vp));
  COUNT("base", base(reinterpret_cast<X *>(nv)));
  COUNT("nonnull_clang", arith(va); base(&x));
  return 0;
}

// ALL: star 1
// ALL-NEXT: cstar 1
// ALL-NEXT: sub 1
// ALL-NEXT: member 1
// ALL-NEXT: refparam 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: arith 1
// ALL-NEXT: base 1
// ALL-NEXT: nonnull_clang 0

// IND: star 0
// IND-NEXT: cstar 0
// IND-NEXT: sub 0
// IND-NEXT: member 0
// IND-NEXT: refparam 1
// IND-NEXT: nonnull 0
// IND-NEXT: arith 0
// IND-NEXT: base 0
// IND-NEXT: nonnull_clang 0

// ACC: star 1
// ACC-NEXT: cstar 1
// ACC-NEXT: sub 1
// ACC-NEXT: member 1
// ACC-NEXT: refparam 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: arith 1
// ACC-NEXT: base 1
// ACC-NEXT: nonnull_clang 0
