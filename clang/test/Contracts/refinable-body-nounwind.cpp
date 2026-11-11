// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@_Z1gi(i32 noundef 6)' --implicit-check-not='@_Z1gi(i32 noundef 7)' --implicit-check-not='@_Z1gi(i32 noundef 8)'
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=quick_enforce -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@_Z1gi(i32 noundef 6)' --implicit-check-not='@_Z1gi(i32 noundef 7)' --implicit-check-not='@_Z1gi(i32 noundef 8)'
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O0 -fcontract-evaluation-semantic=ignore -emit-llvm -o - %s | FileCheck %s --check-prefix=O0
//
// CLANG-824 (fixed): the evaluation semantic of a contract assertion can
// differ between translation units, so a function another unit may also
// define (inline function, template instantiation, constructor and member
// function defined in its class, lambda) must not be found nounwind from
// its body when the body holds a contract assertion: another unit's
// definition, which the linker may keep, may call a throwing violation
// handler.  Here every assertion is ignored (or traps), so none of the
// bodies can throw, but the calls of the functions that may be defined
// elsewhere keep their landing pads (g (1) to g (5)).  The static and the
// external non-inline function have no other definition, and the inline fn
// holds no assertion, so their callers lose theirs (no g (6), g (7), g (8)).
// At -O0 (TryMarkNoThrow runs there too) fi is not nounwind and fe is.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-1.C

int g (int);

[[gnu::noinline]] inline int fi (int x) { contract_assert (x > 0); return x; }

template <typename T>
[[gnu::noinline]] T ft (T x) pre (x > 0) { return x; }

struct S
{
  int v;
  [[gnu::noinline]] S (int x) pre (x > 0) : v (x) { }
  [[gnu::noinline]] int get () const { contract_assert (v > 0); return v; }
};

inline auto lam = [] [[gnu::noinline]] (int x) { contract_assert (x > 0); return x; };

[[gnu::noinline]] static int fs (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] int fe (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] inline int fn (int x) { return x; }

int use_i (int y) { try { return fi (y); } catch (...) { return g (1); } }
int use_t (int y) { try { return ft (y); } catch (...) { return g (2); } }
int use_c (int y) { try { S s (y); return 0; } catch (...) { return g (3); } }
int use_m (const S &s) { try { return s.get (); } catch (...) { return g (4); } }
int use_l (int y) { try { return lam (y); } catch (...) { return g (5); } }
int use_s (int y) { try { return fs (y); } catch (...) { return g (6); } }
int use_e (int y) { try { return fe (y); } catch (...) { return g (7); } }
int use_n (int y) { try { return fn (y); } catch (...) { return g (8); } }

// CHECK-LABEL: define {{.*}}@_Z5use_ii(
// CHECK: invoke {{.*}}@_Z2fii(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 1)
// CHECK: define linkonce_odr {{.*}}@_Z2fii({{.*}}) {{.*}}[[FI:#[0-9]+]]
// CHECK-LABEL: define {{.*}}@_Z5use_ti(
// CHECK: invoke {{.*}}@_Z2ftIiET_S0_(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 2)
// CHECK-LABEL: define {{.*}}@_Z5use_ci(
// CHECK: invoke {{.*}}@_ZN1SC{{[12]}}Ei(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 3)
// CHECK-LABEL: define {{.*}}@_Z5use_mRK1S(
// CHECK: invoke {{.*}}@_ZNK1S3getEv(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 4)
// CHECK-LABEL: define {{.*}}@_Z5use_li(
// CHECK: invoke {{.*}}@_ZNK3lamMUliE_clEi(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 5)
// CHECK: attributes [[FI]] = {
// CHECK-NOT: nounwind
// CHECK-SAME: }

// O0: define {{.*}}@_Z2fei({{.*}}) {{.*}}[[FE:#[0-9]+]]
// O0: define linkonce_odr {{.*}}@_Z2fii({{.*}}) {{.*}}[[FI:#[0-9]+]]
// O0: attributes [[FE]] = { {{.*}}nounwind
// O0: attributes [[FI]] = {
// O0-NOT: nounwind
// O0-SAME: }
