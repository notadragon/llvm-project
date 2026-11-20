// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment:noexcept_observe \
// RUN:   %libcxx_flags -o %t.grp
// RUN: %t.grp | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe \
// RUN:   %libcxx_flags -o %t.no
// RUN: %t.no | FileCheck %s --check-prefix=NONE

// P3100: the alignment check of an access (ub:basic.align.object.alignment
// .store) reports a misaligned load, store, member read or class copy
// through a pointer an explicit cast formed from a character array --
// `*(int *)(buf + o)`, `*(int *)&buf[o]`, a local array, a member, a
// subscript of the cast pointer -- once each, at -O0 and -O2.  CLANG-810:
// the check required the alignment CodeGen derives for the address, which
// a cast keeps from the array's declaration (1, the element's) rather than
// taking the cast-to type's, so none was checked (since upstream's #152575
// for `buf + o`; always for `&buf[o]`).  The cast now records the
// alignment its type requires (P3100CastAlign) for the access through it.
// Controls: the same through a `char *` parameter, a reference binding
// (checked before), the aligned offset, and packed members, whose reduced
// alignment is not a cast's (0).  Stock -fsanitize=alignment misses the
// forms too (CLANG-809, deferred, DECISIONS.md E127 (a)); it is not changed.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-align-char-array-cast*.C;
// gnu_gcc reports every form, a class copy twice there, the binding and
// the load.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
alignas(8) char buf[32];
struct P { int a, b; };
struct __attribute__((packed)) Q { char c; int i; };
Q q;
NI int gload(int o) { return *(int *)(buf + o); }
NI int lload(int o) {
  alignas(8) char lb[16] = {};
  return *(int *)(lb + o);
}
NI void gstore(int o) { *(int *)(buf + o) = 1; }
NI int gmem(int o) { return ((P *)(buf + o))->a; }
NI int gmemb(int o) { return ((P *)(buf + o))->b; }
NI int gmemd(int o) { return (*(P *)(buf + o)).a; }
NI P gcopy(int o) { return *(P *)(buf + o); }
NI int gsub(int o) { return *(int *)&buf[o]; }
NI int gidx0(int o) { return ((int *)(buf + o))[0]; }
NI int gidx(int o, int i) { return ((int *)(buf + o))[i]; }
NI int gsum2(int o) { return *(int *)(buf + o + 4); }
NI long iarr(int o) {
  static int ia[8];
  return *(long *)(ia + o);
}
// Controls.
NI int pload(char *c, int o) { return *(int *)(c + o); }
NI int pmem(char *c, int o) { return ((P *)(c + o))->a; }
NI bool gref(int o) {
  int &r = *(int *)(buf + o);
  return &r == nullptr;
}
NI int qpacked() { return q.i; }
NI int qpackedp(Q *p) { return p->i; }
NI int qcast() { return *(int *)&q.i; }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int volatile one = 1, zero = 0;
  COUNT("gload", gload(one));
  COUNT("lload", lload(one));
  COUNT("gstore", gstore(one));
  COUNT("gmem", gmem(one));
  COUNT("gmemb", gmemb(one));
  COUNT("gmemd", gmemd(one));
  COUNT("gcopy", gcopy(one));
  COUNT("gsub", gsub(one));
  COUNT("gidx0", gidx0(one));
  COUNT("gidx", gidx(one, zero));
  COUNT("gsum2", gsum2(one));
  COUNT("iarr", iarr(one));
  COUNT("pload", pload(buf, one));
  COUNT("pmem", pmem(buf, one));
  COUNT("gref", gref(one));
  COUNT("packed", qpacked(); qpackedp(&q); qcast());
  COUNT("aligned", gload(zero); lload(zero); gstore(zero); gmem(zero);
        gmemb(zero); gmemd(zero); gcopy(zero); gsub(zero); gidx0(zero);
        gidx(zero, zero); gsum2(zero); iarr(zero));
  return 0;
}

// CHECK: gload 1
// CHECK-NEXT: lload 1
// CHECK-NEXT: gstore 1
// CHECK-NEXT: gmem 1
// CHECK-NEXT: gmemb 1
// CHECK-NEXT: gmemd 1
// CHECK-NEXT: gcopy 1
// CHECK-NEXT: gsub 1
// CHECK-NEXT: gidx0 1
// CHECK-NEXT: gidx 1
// CHECK-NEXT: gsum2 1
// CHECK-NEXT: iarr 1
// CHECK-NEXT: pload 1
// CHECK-NEXT: pmem 1
// CHECK-NEXT: gref 1
// CHECK-NEXT: packed 0
// CHECK-NEXT: aligned 0

// NONE: gload 0
// NONE-NEXT: lload 0
// NONE-NEXT: gstore 0
// NONE-NEXT: gmem 0
// NONE-NEXT: gmemb 0
// NONE-NEXT: gmemd 0
// NONE-NEXT: gcopy 0
// NONE-NEXT: gsub 0
// NONE-NEXT: gidx0 0
// NONE-NEXT: gidx 0
// NONE-NEXT: gsum2 0
// NONE-NEXT: iarr 0
// NONE-NEXT: pload 0
// NONE-NEXT: pmem 0
// NONE-NEXT: gref 0
// NONE-NEXT: packed 0
// NONE-NEXT: aligned 0
