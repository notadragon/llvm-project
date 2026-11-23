// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND

// P3100: a subscript of the array a pointer to an array points to,
// `(*pa)[i]` with `pa` null, is an indirection through `pa`, checked as
// `p[i]` checks `p` (DECISIONS.md E83 (c), E97 (c)): once, whatever `i`
// is, at -O0 and -O2 -- under .access where the element is read or
// stored, under .indirection where a reference is bound to it or its
// address alone is formed.  Also through `**ppa` and a two-dimensional
// array.  CLANG-813: the element's address was checked, not null for a
// nonzero `i`, so none of these was reported (at -O2 a non-constant `i`
// was, the optimizer folding the element's address into `pa`).  Controls:
// `pa[0][1]`, `&pa[0][1]` (a subscript of the pointer, already checked on
// `pa`), and a non-null `pa`.  A read faults after its report (caught).
// Stock -fsanitize=null checks the element's address, as for `p[1]`
// (CLANG-745's territory); not changed.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-subscript-deref-array*.C;
// gnu_gcc reports each form once, with these leaves.)

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
NI int *bind1(int (*pa)[4]) { int &r = (*pa)[1]; return &r; }
NI int *bindi(int (*pa)[4], int i) { int &r = (*pa)[i]; return &r; }
NI int *bindp(int (*pa)[4]) { int &r = (*(pa))[2]; return &r; }
NI int read1(int (*pa)[4]) { return (*pa)[1]; }
NI int readi(int (*pa)[4], int i) { return (*pa)[i]; }
NI int read0(int (*pa)[4]) { return (*pa)[0]; }
NI void store1(int (*pa)[4]) { (*pa)[1] = 2; }
NI int *addr1(int (*pa)[4]) { return &(*pa)[1]; }
NI int dd(int (**ppa)[4]) { return (**ppa)[1]; }
NI int md(int (*pm)[2][2]) { return (*pm)[1][1]; }
NI int sub01(int (*pa)[4]) { return pa[0][1]; }
NI int *sub01a(int (*pa)[4]) { return &pa[0][1]; }

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
  int (*volatile nv)[4] = nullptr;
  int (*volatile nm)[2][2] = nullptr;
  int (*pa)[4] = nv;
  int a[4] = {}, m[2][2] = {};
  int (*ap)[4] = &a;
  COUNT("bind1", bind1(nv));
  COUNT("bindi", bindi(nv, 1));
  COUNT("bindp", bindp(nv));
  COUNT("read1", read1(nv));
  COUNT("readi", readi(nv, 1));
  COUNT("read0", read0(nv));
  COUNT("store1", store1(nv));
  COUNT("addr1", addr1(nv));
  COUNT("dd", dd(&pa));
  COUNT("md", md(nm));
  COUNT("sub01", sub01(nv));
  COUNT("sub01a", sub01a(nv));
  COUNT("nonnull", bind1(&a); bindi(&a, 1); bindp(&a); read1(&a);
        readi(&a, 1); read0(&a); store1(&a); addr1(&a); dd(&ap); md(&m);
        sub01(&a); sub01a(&a));
  return 0;
}

// ALL: bind1 1
// ALL-NEXT: bindi 1
// ALL-NEXT: bindp 1
// ALL-NEXT: read1 1
// ALL-NEXT: readi 1
// ALL-NEXT: read0 1
// ALL-NEXT: store1 1
// ALL-NEXT: addr1 1
// ALL-NEXT: dd 1
// ALL-NEXT: md 1
// ALL-NEXT: sub01 1
// ALL-NEXT: sub01a 1
// ALL-NEXT: nonnull 0

// ACC: bind1 0
// ACC-NEXT: bindi 0
// ACC-NEXT: bindp 0
// ACC-NEXT: read1 1
// ACC-NEXT: readi 1
// ACC-NEXT: read0 1
// ACC-NEXT: store1 1
// ACC-NEXT: addr1 0
// ACC-NEXT: dd 1
// ACC-NEXT: md 1
// ACC-NEXT: sub01 1
// ACC-NEXT: sub01a 0
// ACC-NEXT: nonnull 0

// IND: bind1 1
// IND-NEXT: bindi 1
// IND-NEXT: bindp 1
// IND-NEXT: read1 0
// IND-NEXT: readi 0
// IND-NEXT: read0 0
// IND-NEXT: store1 0
// IND-NEXT: addr1 1
// IND-NEXT: dd 0
// IND-NEXT: md 0
// IND-NEXT: sub01 0
// IND-NEXT: sub01a 1
// IND-NEXT: nonnull 0
