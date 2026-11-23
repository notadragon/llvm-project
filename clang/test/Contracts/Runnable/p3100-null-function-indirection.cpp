// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: an indirection through a null pointer to function whose result is
// called, `(*fp)()` (also `(*(fp))()`, `(**fpp)()`, in a template, and
// through `static_cast<F &>`, `(F &)`, `static_cast<F &&>`, a conditional or
// a comma expression), or bound to a reference, `F &r = *fp` (a local, a
// returned reference, an argument, through `static_cast<F &>`, an rvalue
// reference, a conditional, a comma expression), is reported once under
// ub:expr.unary.dereference.nullptr.indirection (CLANG-755, CLANG-757), at
// -O0 and -O2, as a reference bound to `*p` for an object is; so is
// `reinterpret_cast<F &>(*gp)`, bound or called.  .access alone reports
// none.  A plain call `fp()`, which names no indirection, is not checked.  A
// non-null pointer, a function named directly (also chosen by a
// conditional) and a reference to a function (also cast, or returned by a
// call) report nothing.  The handler throws, which skips the call; with no
// report the null call faults, caught as SIGSEGV and resumed with
// siglongjmp, so one run counts every form.  `&*fp` and a discarded `*fp;`
// are in p3100-null-unaccessed-indirection.cpp.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-function-indirection.C,
// with the forms in p3100-null-function-indirection.h, and its -indirection
// and -access variants.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
struct Reported {};
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
  throw Reported{};
}

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

using F = int();
using G = long();
int fn() { return 7; }

#define NI __attribute__((noinline))
NI int call(F *fp) { return (*fp)(); }
NI int call_parens(F *fp) { return (*(fp))(); }
NI int call_twice(F **fpp) { return (**fpp)(); }
template <class T> NI int tcall(T *fp) { return (*fp)(); }
NI int bind(F *fp) {
  F &r = *fp;
  return &r != nullptr;
}
NI F &bind_ret(F *fp) { return *fp; }
NI int take(F &) { return 1; }
NI int bind_arg(F *fp) { return take(*fp); }
NI int bind_cast(F *fp) {
  F &r = static_cast<F &>(*fp);
  return &r != nullptr;
}
NI int call_cast(F *fp) { return static_cast<F &>(*fp)(); }
NI int call_ccast(F *fp) { return ((F &)*fp)(); }
NI int call_rvcast(F *fp) { return static_cast<F &&>(*fp)(); }
NI int call_cond(F *fp, F *gp, bool b) { return (b ? *fp : *gp)(); }
NI int call_comma(F *fp) { return ((void)0, *fp)(); }
NI int bind_rv(F *fp) {
  F &&r = static_cast<F &&>(*fp);
  return &r != nullptr;
}
NI int bind_cond(F *fp, F *gp, bool b) {
  F &r = b ? *fp : *gp;
  return &r != nullptr;
}
NI int bind_comma(F *fp) {
  F &r = ((void)0, *fp);
  return &r != nullptr;
}
NI int bind_rcast(G *gp) {
  F &r = reinterpret_cast<F &>(*gp);
  return &r != nullptr;
}
NI int call_rcast(G *gp) { return reinterpret_cast<F &>(*gp)(); }
// Not checked: a plain call through the pointer names no indirection.
NI int call_plain(F *fp) { return fp(); }
// Controls: nothing to report.
NI int call_named() { return (*fn)(); }
NI int bind_named() {
  F &r = fn;
  return r();
}
NI int bind_ref(F &r) {
  F &r2 = r;
  return r2();
}
NI int call_cond_named(F *fp, bool b) { return (b ? fn : *fp)(); }
NI int call_ref_cast(F &r) { return static_cast<F &>(r)(); }
NI F &id(F &r) { return r; }
NI int call_returned(F &r) { return id(r)(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    if (!sigsetjmp(env, 1)) {                                                  \
      try {                                                                    \
        __VA_ARGS__;                                                           \
      } catch (Reported &) {                                                   \
      }                                                                        \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  F *volatile vfp = nullptr;
  F *fp = vfp;
  F *nfp = fn;
  G *gp = reinterpret_cast<G *>(fp);
  G *ngp = reinterpret_cast<G *>(nfp);
  COUNT("call", call(fp));
  COUNT("call_parens", call_parens(fp));
  COUNT("call_twice", call_twice(&fp));
  COUNT("tcall", tcall(fp));
  COUNT("bind", bind(fp));
  COUNT("bind_ret", (void)&bind_ret(fp));
  COUNT("bind_arg", bind_arg(fp));
  COUNT("bind_cast", bind_cast(fp));
  COUNT("call_cast", call_cast(fp));
  COUNT("call_ccast", call_ccast(fp));
  COUNT("call_rvcast", call_rvcast(fp));
  COUNT("call_cond", call_cond(nfp, fp, false));
  COUNT("call_comma", call_comma(fp));
  COUNT("bind_rv", bind_rv(fp));
  COUNT("bind_cond", bind_cond(nfp, fp, false));
  COUNT("bind_comma", bind_comma(fp));
  COUNT("bind_rcast", bind_rcast(gp));
  COUNT("call_rcast", call_rcast(gp));
  COUNT("call_plain", call_plain(fp));
  COUNT("nonnull", call(nfp); call_twice(&nfp); tcall(nfp); bind(nfp);
        bind_arg(nfp); bind_cast(nfp); call_cast(nfp); call_ccast(nfp);
        call_rvcast(nfp); call_cond(nfp, fp, true); call_comma(nfp);
        bind_rv(nfp); bind_cond(nfp, fp, true); bind_comma(nfp);
        bind_rcast(ngp); call_rcast(ngp); call_plain(nfp); call_named();
        bind_named(); bind_ref(fn); call_cond_named(fp, true);
        call_ref_cast(fn); call_returned(fn));
  return 0;
}

// IND: call 1
// IND-NEXT: call_parens 1
// IND-NEXT: call_twice 1
// IND-NEXT: tcall 1
// IND-NEXT: bind 1
// IND-NEXT: bind_ret 1
// IND-NEXT: bind_arg 1
// IND-NEXT: bind_cast 1
// IND-NEXT: call_cast 1
// IND-NEXT: call_ccast 1
// IND-NEXT: call_rvcast 1
// IND-NEXT: call_cond 1
// IND-NEXT: call_comma 1
// IND-NEXT: bind_rv 1
// IND-NEXT: bind_cond 1
// IND-NEXT: bind_comma 1
// IND-NEXT: bind_rcast 1
// IND-NEXT: call_rcast 1
// IND-NEXT: call_plain 0
// IND-NEXT: nonnull 0

// ACC: call 0
// ACC-NEXT: call_parens 0
// ACC-NEXT: call_twice 0
// ACC-NEXT: tcall 0
// ACC-NEXT: bind 0
// ACC-NEXT: bind_ret 0
// ACC-NEXT: bind_arg 0
// ACC-NEXT: bind_cast 0
// ACC-NEXT: call_cast 0
// ACC-NEXT: call_ccast 0
// ACC-NEXT: call_rvcast 0
// ACC-NEXT: call_cond 0
// ACC-NEXT: call_comma 0
// ACC-NEXT: bind_rv 0
// ACC-NEXT: bind_cond 0
// ACC-NEXT: bind_comma 0
// ACC-NEXT: bind_rcast 0
// ACC-NEXT: call_rcast 0
// ACC-NEXT: call_plain 0
// ACC-NEXT: nonnull 0
