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
// P3100: a member of an object reached through a null pointer, evaluated and
// not accessed, is an indirection through the pointer whose result is not
// accessed, reported once under .indirection (DECISIONS.md E86 (a)): a
// discarded member (`hp->x;`, cast to void, through `(*hp)`, nested, an
// element of an array member, the left operand of a comma, empty, of a
// reference cast of `*sp`, of a base), the object expression of a static
// member, an enumerator or a static member function named through a member,
// a trivial destructor called on a member; a subscript with side effects is
// still evaluated.  A member of a virtual base reads the base's offset
// through the pointer, and a volatile member is read: the .access leaf.  A
// reference and `this` are not checked, nor is a non-null pointer.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-unaccessed-member.h, these
// forms in this order.)

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

struct E {};
struct S {
  int m;
  enum { K = 5 };
  static constexpr int ck = 6;
  static int sf() { return 3; }
};
struct H {
  int x;
  S s;
  int a[2];
  E e;
  int g();
};
struct A { int a; };
struct B { int b; };
struct D : A, B {};
struct V : virtual B { int v; };
struct VH { volatile int vx; };

#define NI __attribute__((noinline))
// Discarded.
NI void disc_member(H *hp) { hp->x; }
NI void disc_member_void(H *hp) { (void)hp->x; }
NI void disc_member_star(H *hp) { (*hp).x; }
NI void disc_member_nested(H *hp) { hp->s.m; }
NI void disc_member_array(H *hp) { hp->a[1]; }
NI void disc_member_comma(H *hp) { (hp->x, 0); }
NI void disc_member_empty(H *hp) { hp->e; }
NI void disc_member_cast(S *sp) { static_cast<S &>(*sp).m; }
NI void disc_member_base(D *dp) { dp->b; }
NI void disc_member_vbase(V *vp) { vp->b; }
// The object expression of a static member, an enumerator, a static
// member function; a trivial destructor called on a member.
NI int smember_static(H *hp) { return hp->s.ck; }
NI int smember_enum(H *hp) { return hp->s.K; }
NI int smember_sf(H *hp) { return hp->s.sf(); }
NI void member_dtor(H *hp) { hp->s.~S(); }
// The subscript is evaluated.
NI int disc_member_side(H *hp) { int i = 0; hp->a[i++]; return i; }
// Read: the .access leaf.
NI void disc_volatile(VH *p) { p->vx; }
// Controls: a reference, `this`.
NI void disc_ref_member(H &r) { r.x; }
int H::g() { x; this->x; (*this).x; return 0; }

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
  H *hp = reinterpret_cast<H *>(vp);
  S *sp = reinterpret_cast<S *>(vp);
  D *dp = reinterpret_cast<D *>(vp);
  V *vbp = reinterpret_cast<V *>(vp);
  VH *vhp = reinterpret_cast<VH *>(vp);
  H h{};
  S s{};
  D d{};
  V v;
  VH vh{};
  COUNT("disc_member", disc_member(hp));
  COUNT("disc_member_void", disc_member_void(hp));
  COUNT("disc_member_star", disc_member_star(hp));
  COUNT("disc_member_nested", disc_member_nested(hp));
  COUNT("disc_member_array", disc_member_array(hp));
  COUNT("disc_member_comma", disc_member_comma(hp));
  COUNT("disc_member_empty", disc_member_empty(hp));
  COUNT("disc_member_cast", disc_member_cast(sp));
  COUNT("disc_member_base", disc_member_base(dp));
  COUNT("disc_member_vbase", disc_member_vbase(vbp));
  COUNT("smember_static", smember_static(hp));
  COUNT("smember_enum", smember_enum(hp));
  COUNT("smember_sf", smember_sf(hp));
  COUNT("member_dtor", member_dtor(hp));
  COUNT("disc_member_side", if (disc_member_side(hp) != 1) reports += 100);
  COUNT("disc_volatile", disc_volatile(vhp));
  COUNT("nonnull", disc_member(&h); disc_member_nested(&h); disc_member_array(&h);
        disc_member_cast(&s); disc_member_base(&d); disc_member_vbase(&v);
        smember_static(&h); member_dtor(&h); disc_volatile(&vh));
  COUNT("controls", disc_ref_member(h); h.g());
  return 0;
}

// ALL: disc_member 1
// ALL-NEXT: disc_member_void 1
// ALL-NEXT: disc_member_star 1
// ALL-NEXT: disc_member_nested 1
// ALL-NEXT: disc_member_array 1
// ALL-NEXT: disc_member_comma 1
// ALL-NEXT: disc_member_empty 1
// ALL-NEXT: disc_member_cast 1
// ALL-NEXT: disc_member_base 1
// ALL-NEXT: disc_member_vbase 1
// ALL-NEXT: smember_static 1
// ALL-NEXT: smember_enum 1
// ALL-NEXT: smember_sf 1
// ALL-NEXT: member_dtor 1
// ALL-NEXT: disc_member_side 1
// ALL-NEXT: disc_volatile 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: controls 0

// IND: disc_member 1
// IND-NEXT: disc_member_void 1
// IND-NEXT: disc_member_star 1
// IND-NEXT: disc_member_nested 1
// IND-NEXT: disc_member_array 1
// IND-NEXT: disc_member_comma 1
// IND-NEXT: disc_member_empty 1
// IND-NEXT: disc_member_cast 1
// IND-NEXT: disc_member_base 1
// IND-NEXT: disc_member_vbase 0
// IND-NEXT: smember_static 1
// IND-NEXT: smember_enum 1
// IND-NEXT: smember_sf 1
// IND-NEXT: member_dtor 1
// IND-NEXT: disc_member_side 1
// IND-NEXT: disc_volatile 0
// IND-NEXT: nonnull 0
// IND-NEXT: controls 0

// ACC: disc_member 0
// ACC-NEXT: disc_member_void 0
// ACC-NEXT: disc_member_star 0
// ACC-NEXT: disc_member_nested 0
// ACC-NEXT: disc_member_array 0
// ACC-NEXT: disc_member_comma 0
// ACC-NEXT: disc_member_empty 0
// ACC-NEXT: disc_member_cast 0
// ACC-NEXT: disc_member_base 0
// ACC-NEXT: disc_member_vbase 1
// ACC-NEXT: smember_static 0
// ACC-NEXT: smember_enum 0
// ACC-NEXT: smember_sf 0
// ACC-NEXT: member_dtor 0
// ACC-NEXT: disc_member_side 0
// ACC-NEXT: disc_volatile 1
// ACC-NEXT: nonnull 0
// ACC-NEXT: controls 0
