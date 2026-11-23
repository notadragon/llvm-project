// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.grp
// RUN: %t.grp | FileCheck %s --check-prefixes=GRP,GRP0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.grp2
// RUN: %t.grp2 | FileCheck %s --check-prefixes=GRP,GRP2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefixes=IND,IND0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefixes=IND,IND2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefixes=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefixes=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t.al
// RUN: %t.al | FileCheck %s --check-prefixes=AL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.store:noexcept_observe \
// RUN:   %libcxx_flags -o %t.al2
// RUN: %t.al2 | FileCheck %s --check-prefixes=AL

// P3100: the null check of a reference bound to `*p`, p a null pointer,
// where the binding has no location of its own -- an implicit conversion
// to the reference: a variable, a `final` class, a member, a downcast at a
// nonzero offset, a call's argument, an overloaded operator's operand, a
// mem-initializer, a default member initializer, an init-capture, a
// structured binding, a range-based for, a `const` reference, a
// parenthesized `*p`, a return -- reports this file and the form's line in
// location(), once each (.indirection); also the conversion to a virtual
// base (.access, it reads the offset through p; the fault after the report
// is caught), the empty base at a nonzero offset (.indirection), and three
// misaligned bindings (the alignment check, .store).  Each run prints the
// number of reports of one form, then how many name this file and line.
// The range-based for over a null `int (*)[2]` binds the range (`*ap`) and
// each element: twice at -O0 (the range and the first element, whose
// address is null), three times at -O2, where the optimizer folds the
// second element's check, on `__begin + 1`, into one of the range pointer
// (CLANG-811, a divergence from gnu_gcc's twice, accepted: DECISIONS.md E128).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-binding-location*.C
// (GCC-874: there these reported an empty location), whose header this
// file's forms are, in this order.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <cstring>
namespace cs = std::contracts;

static unsigned expected;
static int reports, good;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () != cs::assertion_kind::implicit)
    return;
  ++reports;
  const char *f = v.location ().file_name ();
  if (f && std::strstr (f, "p3100-null-binding-location.cpp")
      && v.location ().line () == expected)
    ++good;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

#define NI __attribute__((noinline))
struct S { int m; };
struct F final { int m; };
struct B1 { int x; };
struct B2 { int y; virtual ~B2 () {} };
struct D2 : B1, B2 { int z; };
struct VB { int x; };
struct DV : virtual VB { int y; };
struct EB {};
struct DE2 { int a; };
struct DE : DE2, EB { int c; };
struct P2 { int a, b; };
struct C { int m; };
bool operator== (const C &, const C &) { return true; }
NI void take (int &) {}

NI bool var (int *p) { int &r = *p; return &r == nullptr; } enum { L_var = __LINE__ };
NI bool fin (F *fp) { F &r = *fp; return &r == nullptr; } enum { L_fin = __LINE__ };
NI bool mem (S *sp) { S &r = *sp; return &r.m == nullptr; } enum { L_mem = __LINE__ };
NI bool down (B2 *b2p) { D2 &d = static_cast<D2 &> (*b2p); return &d == nullptr; } enum { L_down = __LINE__ };
NI void arg (int *p) { take (*p); } enum { L_arg = __LINE__ };
NI bool cmp (C *p) { C c{}; return c == *p; } enum { L_cmp = __LINE__ };
struct M { int &r; M (int *p) : r (*p) {} }; enum { L_minit = __LINE__ };
NI bool minit (int *p) { M m (p); return &m.r == nullptr; }
struct N { int *p = nullptr; int &r = *p; }; enum { L_nsdmi = __LINE__ };
NI bool nsdmi () { N n; return &n.r == nullptr; }
NI bool cap (int *p) { auto l = [&r = *p] { return &r == nullptr; }; return l (); } enum { L_cap = __LINE__ };
NI bool sbind (P2 *pp) { auto &[a, b] = *pp; return &a == nullptr; } enum { L_sbind = __LINE__ };
NI int rfor (int (*ap)[2]) { int n = 0; for (int &x : *ap) n += &x == nullptr; return n; } enum { L_rfor = __LINE__ };
NI bool cref (S *sp) { const S &r = *sp; return &r == nullptr; } enum { L_cref = __LINE__ };
NI bool paren (int *p) { int &r = (*p); return &r == nullptr; } enum { L_paren = __LINE__ };
NI int &ret (int *p) { return *p; } enum { L_ret = __LINE__ };
NI bool vbase (DV *p) { VB &b = *p; return &b == nullptr; } enum { L_vbase = __LINE__ };
NI bool ebase (DE *p) { EB &e = *p; return &e == nullptr; } enum { L_ebase = __LINE__ };

#define RUN(name, line, ...)					\
  do								\
    {								\
      reports = good = 0;					\
      expected = line;						\
      if (!sigsetjmp (env, 1))					\
	{							\
	  __VA_ARGS__;						\
	}							\
      std::printf (name " %d %d\n", reports, good);		\
    }								\
  while (0)

alignas (8) char buf[16];

int main ()
{
  std::signal (SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  int *p = nv;
  RUN ("var", L_var, var (p));
  RUN ("fin", L_fin, fin (reinterpret_cast<F *> (p)));
  RUN ("mem", L_mem, mem (reinterpret_cast<S *> (p)));
  RUN ("down", L_down, down (reinterpret_cast<B2 *> (p)));
  RUN ("arg", L_arg, arg (p));
  RUN ("cmp", L_cmp, cmp (reinterpret_cast<C *> (p)));
  RUN ("minit", L_minit, minit (p));
  RUN ("nsdmi", L_nsdmi, nsdmi ());
  RUN ("cap", L_cap, cap (p));
  RUN ("sbind", L_sbind, sbind (reinterpret_cast<P2 *> (p)));
  RUN ("rfor", L_rfor, rfor (reinterpret_cast<int (*)[2]> (p)));
  RUN ("cref", L_cref, cref (reinterpret_cast<S *> (p)));
  RUN ("paren", L_paren, paren (p));
  RUN ("ret", L_ret, (void) &ret (p));
  RUN ("vbase", L_vbase, vbase (reinterpret_cast<DV *> (p)));
  RUN ("ebase", L_ebase, ebase (reinterpret_cast<DE *> (p)));
  // Misaligned: the alignment check of the binding (.store).
  int *volatile mv = reinterpret_cast<int *> (buf + 1);
  RUN ("avar", L_var, var (mv));
  RUN ("aarg", L_arg, arg (mv));
  RUN ("aparen", L_paren, paren (mv));
  int x = 0;
  RUN ("nonnull", 0, var (&x); arg (&x); paren (&x); (void) &ret (&x));
  return 0;
}

// GRP: var 1 1
// GRP-NEXT: fin 1 1
// GRP-NEXT: mem 1 1
// GRP-NEXT: down 1 1
// GRP-NEXT: arg 1 1
// GRP-NEXT: cmp 1 1
// GRP-NEXT: minit 1 1
// GRP-NEXT: nsdmi 1 1
// GRP-NEXT: cap 1 1
// GRP-NEXT: sbind 1 1
// GRP0-NEXT: rfor 2 2
// GRP2-NEXT: rfor 3 3
// GRP-NEXT: cref 1 1
// GRP-NEXT: paren 1 1
// GRP-NEXT: ret 1 1
// GRP-NEXT: vbase 1 1
// GRP-NEXT: ebase 1 1
// GRP-NEXT: avar 0 0
// GRP-NEXT: aarg 0 0
// GRP-NEXT: aparen 0 0
// GRP-NEXT: nonnull 0 0

// IND: var 1 1
// IND-NEXT: fin 1 1
// IND-NEXT: mem 1 1
// IND-NEXT: down 1 1
// IND-NEXT: arg 1 1
// IND-NEXT: cmp 1 1
// IND-NEXT: minit 1 1
// IND-NEXT: nsdmi 1 1
// IND-NEXT: cap 1 1
// IND-NEXT: sbind 1 1
// IND0-NEXT: rfor 2 2
// IND2-NEXT: rfor 3 3
// IND-NEXT: cref 1 1
// IND-NEXT: paren 1 1
// IND-NEXT: ret 1 1
// IND-NEXT: vbase 0 0
// IND-NEXT: ebase 1 1
// IND-NEXT: avar 0 0
// IND-NEXT: aarg 0 0
// IND-NEXT: aparen 0 0
// IND-NEXT: nonnull 0 0

// ACC: var 0 0
// ACC-NEXT: fin 0 0
// ACC-NEXT: mem 0 0
// ACC-NEXT: down 0 0
// ACC-NEXT: arg 0 0
// ACC-NEXT: cmp 0 0
// ACC-NEXT: minit 0 0
// ACC-NEXT: nsdmi 0 0
// ACC-NEXT: cap 0 0
// ACC-NEXT: sbind 0 0
// ACC-NEXT: rfor 0 0
// ACC-NEXT: cref 0 0
// ACC-NEXT: paren 0 0
// ACC-NEXT: ret 0 0
// ACC-NEXT: vbase 1 1
// ACC-NEXT: ebase 0 0
// ACC-NEXT: avar 0 0
// ACC-NEXT: aarg 0 0
// ACC-NEXT: aparen 0 0
// ACC-NEXT: nonnull 0 0

// AL: var 0 0
// AL-NEXT: fin 0 0
// AL-NEXT: mem 0 0
// AL-NEXT: down 0 0
// AL-NEXT: arg 0 0
// AL-NEXT: cmp 0 0
// AL-NEXT: minit 0 0
// AL-NEXT: nsdmi 0 0
// AL-NEXT: cap 0 0
// AL-NEXT: sbind 0 0
// AL-NEXT: rfor 0 0
// AL-NEXT: cref 0 0
// AL-NEXT: paren 0 0
// AL-NEXT: ret 0 0
// AL-NEXT: vbase 0 0
// AL-NEXT: ebase 0 0
// AL-NEXT: avar 1 1
// AL-NEXT: aarg 1 1
// AL-NEXT: aparen 1 1
// AL-NEXT: nonnull 0 0
