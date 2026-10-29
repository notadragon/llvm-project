// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:assume \
// RUN:   -emit-llvm -o - %s | FileCheck %s --check-prefix=ASSUME
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-recover=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -emit-llvm -o - %s | FileCheck %s --check-prefix=CHECKED

// P3100 (UBSan routing): the vptr checks of a static_cast downcast, pointer
// and reference forms (ub:expr.static.cast.downcast.wrong.derived.type,
// ub:expr.static.cast.base.class), and of a conversion to a virtual base are
// off when the check resolves to "assume" -- no vptr runtime call is emitted
// -- and each is instrumented with a checking semantic.  Clang suppresses
// every vptr check of a function the same way, so GCC's GCC-771 does not
// arise here.
// (GCC mirrors: g++.dg/contracts/cpp26/p3100-vptr-downcast-assume-codegen.C
// and p3100-vptr-downcast-codegen.C)

struct S { virtual ~S() {} int s = 1; };
struct T : S { int t = 2; };
struct V : virtual S { int v = 3; };

T *ptr(S *p) { return static_cast<T *>(p); }
T &ref(S &r) { return static_cast<T &>(r); }
S *vbase(V *p) { return p; }

// ASSUME-NOT: __ubsan_handle_dynamic_type_cache_miss
// CHECKED-LABEL: define{{.*}} @_Z3ptrP1S(
// CHECKED: call void @__ubsan_handle_dynamic_type_cache_miss(
// CHECKED-LABEL: define{{.*}} @_Z3refR1S(
// CHECKED: call void @__ubsan_handle_dynamic_type_cache_miss(
// CHECKED-LABEL: define{{.*}} @_Z5vbaseP1V(
// CHECKED: call void @__ubsan_handle_dynamic_type_cache_miss(
