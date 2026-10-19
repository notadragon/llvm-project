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
// P3100: a store to an element of an array of empty class objects through a
// null pointer -- of a member array (`hp->ea[1] = e`, `(*hp).ea[1] = e`, a
// two-dimensional one, a move, `hp->ea[1].operator=(e)`, the first element at
// the object's address) or of the pointer itself (`ep[1] = e`, `ep[0] = e`) --
// writes no object, so the indirection is reported once under .indirection
// (DECISIONS.md E86 (a)), as `hp->e = e` is
// (Runnable/p3100-null-empty-class-copy-extra.cpp).  A non-null pointer is
// not reported.
// (CLANG-771: each store was reported under .access, and `ep[0] = e` under
// both leaves.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-empty-array-store*.C,
// these forms in this order.  GCC checks the element's address for `ep[i]`,
// so it does not report `eptr_store`: the CLANG-745 divergence, which
// gnu_gcc GCC-825 narrows to non-constant subscripts.)

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
struct HA { int x; E ea[2]; E eaa[2][2]; };
struct H0 { E ea[2]; int x; };

#define NI __attribute__((noinline))
NI void earray_store(HA *hp, E &e) { hp->ea[1] = e; }
NI void earray_store_star(HA *hp, E &e) { (*hp).ea[1] = e; }
NI void earray2_store(HA *hp, E &e) { hp->eaa[1][0] = e; }
NI void earray_move_store(HA *hp, E &e) { hp->ea[1] = static_cast<E &&>(e); }
NI void earray_call(HA *hp, E &e) { hp->ea[1].operator=(e); }
NI void earray0_store(H0 *hp, E &e) { hp->ea[0] = e; }
NI void eptr_store(E *ep, E &e) { ep[1] = e; }
NI void eptr0_store(E *ep, E &e) { ep[0] = e; }

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
  HA *hap = reinterpret_cast<HA *>(vp);
  H0 *h0p = reinterpret_cast<H0 *>(vp);
  E *ep = reinterpret_cast<E *>(vp);
  E e, ea[2];
  HA ha{};
  H0 h0{};
  COUNT("earray_store", earray_store(hap, e));
  COUNT("earray_store_star", earray_store_star(hap, e));
  COUNT("earray2_store", earray2_store(hap, e));
  COUNT("earray_move_store", earray_move_store(hap, e));
  COUNT("earray_call", earray_call(hap, e));
  COUNT("earray0_store", earray0_store(h0p, e));
  COUNT("eptr_store", eptr_store(ep, e));
  COUNT("eptr0_store", eptr0_store(ep, e));
  COUNT("nonnull", earray_store(&ha, e); earray_store_star(&ha, e);
        earray2_store(&ha, e); earray_move_store(&ha, e); earray_call(&ha, e);
        earray0_store(&h0, e); eptr_store(ea, e); eptr0_store(ea, e));
  return 0;
}

// ALL: earray_store 1
// ALL-NEXT: earray_store_star 1
// ALL-NEXT: earray2_store 1
// ALL-NEXT: earray_move_store 1
// ALL-NEXT: earray_call 1
// ALL-NEXT: earray0_store 1
// ALL-NEXT: eptr_store 1
// ALL-NEXT: eptr0_store 1
// ALL-NEXT: nonnull 0

// IND: earray_store 1
// IND-NEXT: earray_store_star 1
// IND-NEXT: earray2_store 1
// IND-NEXT: earray_move_store 1
// IND-NEXT: earray_call 1
// IND-NEXT: earray0_store 1
// IND-NEXT: eptr_store 1
// IND-NEXT: eptr0_store 1
// IND-NEXT: nonnull 0

// ACC: earray_store 0
// ACC-NEXT: earray_store_star 0
// ACC-NEXT: earray2_store 0
// ACC-NEXT: earray_move_store 0
// ACC-NEXT: earray_call 0
// ACC-NEXT: earray0_store 0
// ACC-NEXT: eptr_store 0
// ACC-NEXT: eptr0_store 0
// ACC-NEXT: nonnull 0
