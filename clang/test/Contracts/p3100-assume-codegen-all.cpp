// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts -fcontracts-p3100 -w -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts -w -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-assume-codegen-all.json -w -emit-llvm -o - %s | FileCheck %s --check-prefix=OBSERVE

// P3100: every implicit check resolved to assume emits no check at all.  The
// builtin default resolves implicit assertions to assume, so with no
// configuration file each site below -- division and remainder by zero and
// overflow, shift, a known-bound subscript, an enum conversion out of range,
// a float-to-int conversion, flowing off the end of a non-void function,
// signed overflow, a null dereference and a bool load -- compiles to no
// violation call and no sanitizer call (the per-check *-assume tests cover a
// few of these one at a time).  Compiled at -O0, so nothing here is the
// optimizer's doing.
//
// The assume semantic is "today's semantics" (P3100), so the first RUN line
// must match the second, which does not enable P3100 at all: in particular
// the only trap is the one Clang has always emitted at -O0 for flowing off
// the end of a non-void function.  (GCC emits no trap there, and its mirror
// says so.)  The OBSERVE run is the control: every function calls the
// violation entry point there, so the absence of one means something.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-codegen-all.C)

enum Pl { P0, P1 };
int g[4];
int divz(int a, int b) { return a / b; }
int modz(int a, int b) { return a % b; }
int shl(int a, int b) { return a << b; }
int bnd(int i) { return g[i]; }
Pl ecast(int i) { return static_cast<Pl>(i); }
int fpint(double d) { return (int)d; }
int flow(int x) { if (x) return 1; }
int ovf(int a, int b) { return a + b; }
int nul(int *p) { return *p; }
bool ldb(bool *p) { return *p; }

// CHECK-NOT: contract_violation
// CHECK-NOT: __ubsan
// CHECK-NOT: llvm.trap
// CHECK-LABEL: define {{.*}}@_Z4flowi(
// CHECK:       call void @llvm.trap()
// CHECK-NOT: contract_violation
// CHECK-NOT: __ubsan
// CHECK-NOT: call void @llvm.trap

// OBSERVE-LABEL: define {{.*}}@_Z4divzii(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z4modzii(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z3shlii(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z3bndi(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z5ecasti(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z5fpintd(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z4flowi(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z3ovfii(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z3nulPi(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
// OBSERVE-LABEL: define {{.*}}@_Z3ldbPb(
// OBSERVE:       call void @__cxa_contract_violation_implicit_observe
