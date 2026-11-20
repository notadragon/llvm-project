// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.ign
// RUN: %t.ign | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.ign2
// RUN: %t.ign2 | FileCheck %s --check-prefix=NONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,alignment -fsanitize-semantic=null:noexcept_observe,alignment:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.al
// RUN: %t.al | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,alignment -fsanitize-semantic=null:noexcept_observe,alignment:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.al2
// RUN: %t.al2 | FileCheck %s --check-prefix=ALL

// P3100: with -fsanitize=null routed to the contract-violation handler (and
// -fsanitize=null,alignment), a dynamic_cast to a reference type of the
// object a null pointer points to -- the forms of
// p3100-null-dynamic-cast-reference.cpp -- is reported once at -O0 and -O2
// where the P3100 check is made (the group, or .access: E102 (a)), the
// sanitizer's check of the same object replaced (DECISIONS.md E111 (a),
// E120).  Where the P3100 leaf is not configured (.indirection alone, or
// the group ignored) the sanitizer's check would be made instead (E111
// (a)), but stock -fsanitize=null has none for the operand of a
// dynamic_cast to a reference (CLANG-812, stock Clang 18..trunk;
// deferred, DECISIONS.md E129): `dynamic_cast<W &>(*vp)`, `vp[1]`, `*(vp + 1)`
// report 0 here where gnu_gcc reports 1 (`vp[1]` and `*(vp + 1)` 0 there
// too with the group ignored: the sanitizer tests the element's address,
// E115 (a)).  The member (its member access), the reference parameter (its
// binding) and the base conversion (the object checked before it) are
// reported once in every run.  A fault after a report, or a std::bad_cast,
// is caught.
// Mirror: gcc/testsuite/g++.dg/ubsan/
// p3100-null-dynamic-cast-reference-sanitize-null*.C (GCC-875; there the
// -fsanitize=alignment runs report the binding's alignment check's null
// edge again, accepted under E120).

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

// NONE: star 0
// NONE-NEXT: cstar 0
// NONE-NEXT: sub 0
// NONE-NEXT: member 1
// NONE-NEXT: refparam 1
// NONE-NEXT: nonnull 0
// NONE-NEXT: arith 0
// NONE-NEXT: base 1
// NONE-NEXT: nonnull_clang 0
