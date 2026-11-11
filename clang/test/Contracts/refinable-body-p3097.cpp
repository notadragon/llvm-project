// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s
//
// The refinable-body rule for P3097: a function that calls a virtual
// function through its contract wrapper (internal to this translation
// unit), or checks the virtual function's contracts at the call (a
// C-variadic function, a destructor), holds evaluations that another
// translation unit's definition of the function may make differently or not
// at all (it may not use P3097), so it is never settled
// (CurFnHoldsUnsettledContract).  Here every virtual function is noexcept,
// so a throwing violation handler would terminate in every copy, and the
// nounwind TryMarkNoThrow used to find for h, hv and hd was right; the rule
// costs their callers a landing pad.  It is kept because the same fact
// feeds P4405's proof shadows (ContractShadowDecide.cpp, rule 1), which may
// not rely on what an inlined copy of h does unless the real caller inlines
// it too.  Without P3097 (hn) a virtual call holds no evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-p3097.C (h,
// hv, hd, hn) and the P3097 half of
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-label.C (h_settled,
// h_open; GCC skips the mark for a wrapper whose evaluations are all
// settled -- Clang marks every P3097 caller)

int g (int);

struct B
{
  virtual int vf (int x) noexcept post (r: r > 0);
  virtual int vv (int x, ...) noexcept pre (x > 0);
  virtual ~B () pre (true);
};

struct N
{
  virtual int vn (int x) noexcept;
};

[[gnu::noinline]] inline int h (B *p, int x) { return p->vf (x); }
[[gnu::noinline]] inline int hv (B *p, int x) { return p->vv (x, 1); }
[[gnu::noinline]] inline void hd (B *p) { p->~B (); }
[[gnu::noinline]] inline int hn (N *p, int x) { return p->vn (x); }

int use_h (B *p, int y) { try { return h (p, y); } catch (...) { return g (1); } }
int use_hv (B *p, int y) { try { return hv (p, y); } catch (...) { return g (2); } }
int use_hd (B *p) { try { hd (p); return 0; } catch (...) { return g (3); } }
int use_hn (N *p, int y) { try { return hn (p, y); } catch (...) { return g (4); } }

// CHECK-LABEL: define {{.*}}@_Z5use_hP1Bi(
// CHECK: invoke {{.*}}@_Z1hP1Bi(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 1)
// CHECK-LABEL: define {{.*}}@_Z6use_hvP1Bi(
// CHECK: invoke {{.*}}@_Z2hvP1Bi(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 2)
// CHECK-LABEL: define {{.*}}@_Z6use_hdP1B(
// CHECK: invoke {{.*}}@_Z2hdP1B(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 3)
// CHECK-LABEL: define {{.*}}@_Z6use_hnP1Ni(
// CHECK-NOT: @_Z1gi(i32 noundef 4)
// CHECK: ret
