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
// P3100: a conversion to a virtual base reads the base's offset through the
// object, so by DECISIONS.md E94 (b) every conversion of a glvalue to a
// virtual base through a null pointer is the .access leaf, reported once,
// whatever its result is used for: a reference bound to it, its address
// taken, discarded -- of a member object (`E &r = hp->ve`,
// `&static_cast<E &>(hp->ve)`, `static_cast<E &>(hp->ve);`, `(*hp).ve`,
// `hp->*pm`), of an element of a member array (`hp->va[1]`), of a non-empty
// virtual base (`B &r = hp->vb`), of an element `vp[i]` of the null pointer
// itself (checked on `vp`, as an accessed `vp[i]` is).  A member of such a
// base read, called or its address taken is .access too.  `E *q = &hp->ve`
// converts the pointer `&hp->ve`, which is not null: the indirection
// `hp->ve` under `&` is reported, under .indirection.  A non-null pointer is
// not reported.
// (CLANG-772: the conversions of a member object, of an element of a
// member array and through `->*` were reported under .indirection, and
// `vp[1]` converted was not checked.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-vbase-member-bind*.C, all
// but `E *q = &hp->ve`, which GCC reports under .indirection by DECISIONS.md
// E96 (a).)

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
struct B {
  int b;
  void f() {}
};
struct VE : virtual E { int v; };
struct VB : virtual B { int v; };
struct HV { int x; VE ve; VE va[2]; VB vb; };

#define NI __attribute__((noinline))
NI E *hvmember_bind(HV *hp) { E &r = hp->ve; return &r; }
NI E *hvmember_addr(HV *hp) { return &static_cast<E &>(hp->ve); }
NI void hvmember_disc(HV *hp) { static_cast<E &>(hp->ve); }
NI E *hvstar_bind(HV *hp) { E &r = (*hp).ve; return &r; }
NI E *hvmptr_bind(HV *hp, VE HV::*pm) { E &r = hp->*pm; return &r; }
NI E *hvarray_bind(HV *hp) { E &r = hp->va[1]; return &r; }
NI void hvarray_disc(HV *hp) { static_cast<E &>(hp->va[1]); }
NI B *hvbase_bind(HV *hp) { B &r = hp->vb; return &r; }
NI int hvbase_read(HV *hp) { return hp->vb.b; }
NI void hvbase_call(HV *hp) { hp->vb.f(); }
NI int *hvbase_addr(HV *hp) { return &hp->vb.b; }
NI E *vsub_bind(VE *vp) { E &r = vp[1]; return &r; }
NI E *vsub0_bind(VE *vp) { E &r = vp[0]; return &r; }
NI void vsub_disc(VE *vp) { static_cast<E &>(vp[1]); }
NI E *hvmember_ptr(HV *hp) { E *q = &hp->ve; return q; }

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
  HV *hvp = reinterpret_cast<HV *>(vp);
  VE *vep = reinterpret_cast<VE *>(vp);
  HV hv{};
  VE va[2];
  COUNT("hvmember_bind", hvmember_bind(hvp));
  COUNT("hvmember_addr", hvmember_addr(hvp));
  COUNT("hvmember_disc", hvmember_disc(hvp));
  COUNT("hvstar_bind", hvstar_bind(hvp));
  COUNT("hvmptr_bind", hvmptr_bind(hvp, &HV::ve));
  COUNT("hvarray_bind", hvarray_bind(hvp));
  COUNT("hvarray_disc", hvarray_disc(hvp));
  COUNT("hvbase_bind", hvbase_bind(hvp));
  COUNT("hvbase_read", hvbase_read(hvp));
  COUNT("hvbase_call", hvbase_call(hvp));
  COUNT("hvbase_addr", hvbase_addr(hvp));
  COUNT("vsub_bind", vsub_bind(vep));
  COUNT("vsub0_bind", vsub0_bind(vep));
  COUNT("vsub_disc", vsub_disc(vep));
  COUNT("hvmember_ptr", hvmember_ptr(hvp));
  COUNT("nonnull", hvmember_bind(&hv); hvmember_addr(&hv); hvmember_disc(&hv);
        hvstar_bind(&hv); hvmptr_bind(&hv, &HV::ve); hvarray_bind(&hv);
        hvarray_disc(&hv); hvbase_bind(&hv); hvbase_read(&hv); hvbase_call(&hv);
        hvbase_addr(&hv); vsub_bind(va); vsub0_bind(va); vsub_disc(va);
        hvmember_ptr(&hv));
  return 0;
}

// ALL: hvmember_bind 1
// ALL-NEXT: hvmember_addr 1
// ALL-NEXT: hvmember_disc 1
// ALL-NEXT: hvstar_bind 1
// ALL-NEXT: hvmptr_bind 1
// ALL-NEXT: hvarray_bind 1
// ALL-NEXT: hvarray_disc 1
// ALL-NEXT: hvbase_bind 1
// ALL-NEXT: hvbase_read 1
// ALL-NEXT: hvbase_call 1
// ALL-NEXT: hvbase_addr 1
// ALL-NEXT: vsub_bind 1
// ALL-NEXT: vsub0_bind 1
// ALL-NEXT: vsub_disc 1
// ALL-NEXT: hvmember_ptr 1
// ALL-NEXT: nonnull 0

// IND: hvmember_bind 0
// IND-NEXT: hvmember_addr 0
// IND-NEXT: hvmember_disc 0
// IND-NEXT: hvstar_bind 0
// IND-NEXT: hvmptr_bind 0
// IND-NEXT: hvarray_bind 0
// IND-NEXT: hvarray_disc 0
// IND-NEXT: hvbase_bind 0
// IND-NEXT: hvbase_read 0
// IND-NEXT: hvbase_call 0
// IND-NEXT: hvbase_addr 0
// IND-NEXT: vsub_bind 0
// IND-NEXT: vsub0_bind 0
// IND-NEXT: vsub_disc 0
// IND-NEXT: hvmember_ptr 1
// IND-NEXT: nonnull 0

// ACC: hvmember_bind 1
// ACC-NEXT: hvmember_addr 1
// ACC-NEXT: hvmember_disc 1
// ACC-NEXT: hvstar_bind 1
// ACC-NEXT: hvmptr_bind 1
// ACC-NEXT: hvarray_bind 1
// ACC-NEXT: hvarray_disc 1
// ACC-NEXT: hvbase_bind 1
// ACC-NEXT: hvbase_read 1
// ACC-NEXT: hvbase_call 1
// ACC-NEXT: hvbase_addr 1
// ACC-NEXT: vsub_bind 1
// ACC-NEXT: vsub0_bind 1
// ACC-NEXT: vsub_disc 1
// ACC-NEXT: hvmember_ptr 0
// ACC-NEXT: nonnull 0
