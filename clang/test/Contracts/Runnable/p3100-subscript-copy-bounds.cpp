// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:noexcept_observe \
// RUN:   %libcxx_flags -o %t.sub
// RUN: %t.sub | FileCheck %s --check-prefix=SUB
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:noexcept_observe \
// RUN:   %libcxx_flags -o %t.sub2
// RUN: %t.sub2 | FileCheck %s --check-prefix=SUB
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe \
// RUN:   %libcxx_flags -o %t.inv
// RUN: %t.inv | FileCheck %s --check-prefix=INV
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:noexcept_observe \
// RUN:   %libcxx_flags -o %t.inv2
// RUN: %t.inv2 | FileCheck %s --check-prefix=INV
// P3100: a class copied from the element one past the end of an array of
// known bound (`S s2 = sa[N]`, `s = sa[N]`, by value) reads it, so it is
// ub:expr.add.out.of.bounds.known.subscript, as a read of a scalar element
// and a store to the element are, reported once.  An element in range is
// not reported.  An empty class copied from or stored to the element
// (`E e2 = ea[N]`, `ea[N] = e`, in parentheses, an empty base of a
// non-empty element, a subscript of known-bound pointer arithmetic
// `(ea + 1)[i]`) reads or writes no object, so it is the indirection past
// the end, ub:expr.unary.dereference.invalid.known, as `&ea[N]` is
// (DECISIONS.md E95 (a), E86 (a)'s rule), reported once.  Then GCC's extra
// empty forms: by value, returned, moved, move-stored and
// `ea[N].operator=(e)`, each .invalid.known once, and one further past
// (`ea[N + 1]`), checked as `&ea[N + 1]` is, reported once under either
// leaf.
// (CLANG-773: the class copies were reported under .invalid.known, the
// qualification conversion of the copy's source hiding the subscript from
// EmitCheckedLValue.  CLANG-774: the empty stores were reported under
// .known.subscript.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds-copy*.C, whose
// header has these forms in this order.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

struct E {};
struct S { int a, b; };
struct EB {};
struct D : EB { int x; };

#define NI __attribute__((noinline))
S sa[2];
E ea[2];
int ia[2];
D da[2];
NI void sbyval(S) {}
NI void ebyval(E) {}
NI void scopy(int i) { S s2 = sa[i]; (void)s2; }
NI void sfrom(int i, S &s) { s = sa[i]; }
NI void sarg(int i) { sbyval(sa[i]); }
NI void sstore(int i, S &s) { sa[i] = s; }
NI int iread(int i) { return ia[i]; }
NI void ecopy(int i) { E e2 = ea[i]; (void)e2; }
NI void estore(int i, E &e) { ea[i] = e; }
NI void eparenstore(int i, E &e) { (ea[i]) = e; }
NI void bcopy(int i) { EB b = da[i]; (void)b; }
NI void bstore(int i, EB &b) { static_cast<EB &>(da[i]) = b; }
NI void earithcopy(int i) { E e2 = (ea + 1)[i]; (void)e2; }
NI void earithstore(int i, E &e) { (ea + 1)[i] = e; }
// GCC's: an empty element copied by value, returned, moved, and stored to
// by a move or an explicit call.
NI void earg(int i) { ebyval(ea[i]); }
NI E eret(int i) { return ea[i]; }
NI void emove(int i) { E e2 = static_cast<E &&>(ea[i]); (void)e2; }
NI void emovestore(int i, E &e) { ea[i] = static_cast<E &&>(e); }
NI void ecall(int i, E &e) { ea[i].operator=(e); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  volatile int one = 1, two = 2;
  volatile int three = 3;
  S s{};
  E e;
  EB b;
  COUNT("scopy", scopy(two));
  COUNT("sfrom", sfrom(two, s));
  COUNT("sarg", sarg(two));
  COUNT("sstore", sstore(two, s));
  COUNT("iread", iread(two));
  COUNT("ecopy", ecopy(two));
  COUNT("estore", estore(two, e));
  COUNT("eparenstore", eparenstore(two, e));
  COUNT("bcopy", bcopy(two));
  COUNT("bstore", bstore(two, b));
  COUNT("earithcopy", earithcopy(one));
  COUNT("earithstore", earithstore(one, e));
  COUNT("inrange", scopy(one); sfrom(one, s); sarg(one); sstore(one, s);
        iread(one); ecopy(one); estore(one, e); eparenstore(one, e);
        bcopy(one); bstore(one, b); earithcopy(0); earithstore(0, e));
  COUNT("earg", earg(two));
  COUNT("eret", eret(two));
  COUNT("emove", emove(two));
  COUNT("emovestore", emovestore(two, e));
  COUNT("ecall", ecall(two, e));
  COUNT("ecopy3", ecopy(three));
  COUNT("estore3", estore(three, e));
  COUNT("inrange_extra", earg(one); eret(one); emove(one);
        emovestore(one, e); ecall(one, e));
  return 0;
}

// SUB: scopy 1
// SUB-NEXT: sfrom 1
// SUB-NEXT: sarg 1
// SUB-NEXT: sstore 1
// SUB-NEXT: iread 1
// SUB-NEXT: ecopy 0
// SUB-NEXT: estore 0
// SUB-NEXT: eparenstore 0
// SUB-NEXT: bcopy 0
// SUB-NEXT: bstore 0
// SUB-NEXT: earithcopy 0
// SUB-NEXT: earithstore 0
// SUB-NEXT: inrange 0
// SUB-NEXT: earg 0
// SUB-NEXT: eret 0
// SUB-NEXT: emove 0
// SUB-NEXT: emovestore 0
// SUB-NEXT: ecall 0
// SUB-NEXT: ecopy3 1
// SUB-NEXT: estore3 1
// SUB-NEXT: inrange_extra 0

// INV: scopy 0
// INV-NEXT: sfrom 0
// INV-NEXT: sarg 0
// INV-NEXT: sstore 0
// INV-NEXT: iread 0
// INV-NEXT: ecopy 1
// INV-NEXT: estore 1
// INV-NEXT: eparenstore 1
// INV-NEXT: bcopy 1
// INV-NEXT: bstore 1
// INV-NEXT: earithcopy 1
// INV-NEXT: earithstore 1
// INV-NEXT: inrange 0
// INV-NEXT: earg 1
// INV-NEXT: eret 1
// INV-NEXT: emove 1
// INV-NEXT: emovestore 1
// INV-NEXT: ecall 1
// INV-NEXT: ecopy3 1
// INV-NEXT: estore3 1
// INV-NEXT: inrange_extra 0
