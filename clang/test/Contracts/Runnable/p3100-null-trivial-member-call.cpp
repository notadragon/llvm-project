// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: a trivial special member function called explicitly through a null
// pointer.  A trivial destructor call evaluates its object expression and
// accesses nothing, like a pseudo-destructor call: `sp->~S()`,
// `(*sp).~S()`, `sp->S::~S()`, the destructor of a base at a nonzero offset
// (`dp->S::~S()`, `(*dp).S::~S()`, p checked before the conversion) and one
// in a template are each reported once under .indirection (CLANG-759); a
// virtual base's (`vp->S::~S()`) loads the base's offset through vp, the
// .access leaf.  A trivial assignment called through the pointer,
// `sp->operator=(s)`, from a temporary, or a base's `dp->S::operator=(s)`,
// stores through it as `*sp = s` and `(*sp).operator=(s)` do, so it is
// reported once under .access (CLANG-760).  `this`, a declared object and
// the address of an object are not checked; neither is a non-null pointer.
// A fault after the report is caught as SIGSEGV and resumed with
// siglongjmp, so one run counts every form.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-trivial-member-call*.C,
// the same forms and runs.)

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

struct S {
  int m;
  void self();
};
struct A { int a; };
struct D : A, S {};
struct V : virtual S {};

#define NI __attribute__((noinline))
NI void sdtor(S *sp) { sp->~S(); }
NI void sdtor_star(S *sp) { (*sp).~S(); }
NI void sdtor_qual(S *sp) { sp->S::~S(); }
NI void bdtor(D *dp) { dp->S::~S(); }
NI void bdtor_star(D *dp) { (*dp).S::~S(); }
template <class T> NI void tdtor(T *p) { p->~T(); }
NI void vdtor(V *vp) { vp->S::~S(); }
NI void assign(S *sp, S &s) { sp->operator=(s); }
NI void assign_tmp(S *sp) { sp->operator=(S()); }
NI void assign_base(D *dp, S &s) { dp->S::operator=(s); }
NI void assign_star(S *sp, S &s) { (*sp).operator=(s); }
NI void assign_op(S *sp, S &s) { *sp = s; }
// Controls.
void S::self() {
  this->~S();
  S::~S();
  this->operator=(*this);
}
NI void local() {
  S s{};
  s.~S();
  (&s)->~S();
  (&s)->operator=(s);
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
  int *volatile vp = nullptr;
  S *sp = reinterpret_cast<S *>(vp);
  D *dp = reinterpret_cast<D *>(vp);
  V *vbp = reinterpret_cast<V *>(vp);
  S s{};
  D d{};
  V v;
  COUNT("sdtor", sdtor(sp));
  COUNT("sdtor_star", sdtor_star(sp));
  COUNT("sdtor_qual", sdtor_qual(sp));
  COUNT("bdtor", bdtor(dp));
  COUNT("bdtor_star", bdtor_star(dp));
  COUNT("tdtor", tdtor(sp));
  COUNT("vdtor", vdtor(vbp));
  COUNT("assign", assign(sp, s));
  COUNT("assign_tmp", assign_tmp(sp));
  COUNT("assign_base", assign_base(dp, s));
  COUNT("assign_star", assign_star(sp, s));
  COUNT("assign_op", assign_op(sp, s));
  COUNT("nonnull", sdtor(&s); sdtor_star(&s); bdtor(&d); bdtor_star(&d);
        tdtor(&s); vdtor(&v); assign(&s, s); assign_base(&d, s));
  COUNT("controls", s.self(); local());
  return 0;
}

// ALL: sdtor 1
// ALL-NEXT: sdtor_star 1
// ALL-NEXT: sdtor_qual 1
// ALL-NEXT: bdtor 1
// ALL-NEXT: bdtor_star 1
// ALL-NEXT: tdtor 1
// ALL-NEXT: vdtor 1
// ALL-NEXT: assign 1
// ALL-NEXT: assign_tmp 1
// ALL-NEXT: assign_base 1
// ALL-NEXT: assign_star 1
// ALL-NEXT: assign_op 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: controls 0

// IND: sdtor 1
// IND-NEXT: sdtor_star 1
// IND-NEXT: sdtor_qual 1
// IND-NEXT: bdtor 1
// IND-NEXT: bdtor_star 1
// IND-NEXT: tdtor 1
// IND-NEXT: vdtor 0
// IND-NEXT: assign 0
// IND-NEXT: assign_tmp 0
// IND-NEXT: assign_base 0
// IND-NEXT: assign_star 0
// IND-NEXT: assign_op 0
// IND-NEXT: nonnull 0
// IND-NEXT: controls 0

// ACC: sdtor 0
// ACC-NEXT: sdtor_star 0
// ACC-NEXT: sdtor_qual 0
// ACC-NEXT: bdtor 0
// ACC-NEXT: bdtor_star 0
// ACC-NEXT: tdtor 0
// ACC-NEXT: vdtor 1
// ACC-NEXT: assign 1
// ACC-NEXT: assign_tmp 1
// ACC-NEXT: assign_base 1
// ACC-NEXT: assign_star 1
// ACC-NEXT: assign_op 1
// ACC-NEXT: nonnull 0
// ACC-NEXT: controls 0
