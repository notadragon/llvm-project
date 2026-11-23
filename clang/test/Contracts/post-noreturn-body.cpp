// RUN: %clang_cc1 -std=c++26 -fcontracts -triple x86_64-unknown-linux-gnu \
// RUN:   -fcxx-exceptions -fexceptions -emit-llvm -o - %s \
// RUN:   | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -triple x86_64-unknown-linux-gnu \
// RUN:   -fcxx-exceptions -fexceptions -O2 -disable-llvm-passes -emit-llvm \
// RUN:   -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -DP3097 \
// RUN:   -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions \
// RUN:   -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,P3097
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -DP3097 \
// RUN:   -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions \
// RUN:   -O2 -disable-llvm-passes -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefixes=CHECK,P3097

// A postcondition on a function whose body never returns -- an infinite
// loop, a throw, a [[noreturn]] call, __builtin_unreachable -- is never
// evaluated: nothing reaches the return block.  Its checks used to be
// emitted into that block anyway, splitting the epilogue, and when
// FinishFunction erased the unreachable block the 'ret' left behind still
// used the return-slot load defined in it ("Uses remain when a value is
// destroyed").  Now nothing is emitted for the postconditions there, for
// every kind of return type; a precondition and a contract_assert in the body
// are emitted as usual, and a body that returns on one path still checks the
// postcondition on that path.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/post-noreturn-body.C,
// post-noreturn-body-O2.C, post-noreturn-body-p3097.C and
// post-noreturn-body-p3097-O2.C
// and EDG's codegen/post_noreturn_body

[[noreturn]] void die();

// CHECK-LABEL: define {{.*}}i32 @_Z8int_loopv()
// CHECK-NOT: contract
// CHECK: while.body:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
int int_loop() post(true) { while (true) {} }

// CHECK-LABEL: define {{.*}}void @_Z9void_loopv()
// CHECK-NOT: contract
// CHECK: while.body:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
void void_loop() post(true) { while (true) {} }

// CHECK-LABEL: define {{.*}}i32 @_Z10named_loopv()
// CHECK-NOT: contract
// CHECK: while.body:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
int named_loop() post(r: r > 0) { while (true) {} }

// CHECK-LABEL: define {{.*}}i32 @_Z9int_throwv()
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
int int_throw() post(true) { throw 1; }

// CHECK-LABEL: define {{.*}}i32 @_Z17int_noreturn_callv()
// CHECK-NOT: contract
// CHECK: call void @_Z3diev()
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
int int_noreturn_call() post(true) { die(); }

// CHECK-LABEL: define {{.*}}i32 @_Z15int_unreachablev()
// CHECK-NEXT: entry:
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
int int_unreachable() post(true) { __builtin_unreachable(); }

// CHECK-LABEL: define {{.*}}i32 @_Z12noreturn_intv()
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
[[noreturn]] int noreturn_int() post(true) { throw 1; }

// A reference, a class returned in a register, a class returned through an
// sret pointer whose destructor guards the result, and a complex number.

// CHECK-LABEL: define {{.*}}ptr @_Z9ref_throwv()
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
int &ref_throw() post(r: r > 0) { throw 1; }

struct Small { int a; };
// CHECK-LABEL: define {{.*}}i32 @_Z11small_throwv()
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
Small small_throw() post(r: r.a > 0) { throw 1; }

struct Big { Big(); ~Big(); int a; };
// CHECK-LABEL: define {{.*}}void @_Z9big_throwv(ptr {{.*}}sret(%struct.Big)
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
Big big_throw() post(true) { throw 1; }

// CHECK-LABEL: define {{.*}}{ double, double } @_Z13complex_throwv()
// CHECK-NOT: contract
// CHECK: call void @__cxa_throw(
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
_Complex double complex_throw() post(true) { throw 1; }

// A result-named postcondition on a body that flows off the end without
// returning a value (emitted, never called), and on class returns, in a
// register and through an sret pointer, whose body is a [[noreturn]] call.
// CHECK-LABEL: define {{.*}}i32 @_Z8flow_offv()
// CHECK-NOT: contract
// CHECK: unreachable
// CHECK-NOT: contract
// CHECK: {{^}}}
int flow_off() post(r: true) { }

// CHECK-LABEL: define {{.*}}i32 @_Z9small_diev()
// CHECK-NOT: contract
// CHECK: call void @_Z3diev()
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
Small small_die() post(r: r.a > 0) { die(); }

// CHECK-LABEL: define {{.*}}void @_Z7big_diev(ptr {{.*}}sret(%struct.Big)
// CHECK-NOT: contract
// CHECK: call void @_Z3diev()
// CHECK-NEXT: unreachable
// CHECK-NEXT: }
Big big_die() post(r: r.a > 0) { die(); }

// The precondition is checked; the postcondition is not emitted.
// CHECK-LABEL: define {{.*}}i32 @_Z8pre_posti(
// CHECK: icmp sgt i32 %{{.*}}, 0
// CHECK: call void @__cxa_contract_violation_pre_enforce_pf(
// CHECK-NOT: contract_violation_post
// CHECK: while.body:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
int pre_post(int x) pre(x > 0) post(true) { while (true) {} }

// The contract_assert in the loop is checked; the postcondition is not
// emitted.
// CHECK-LABEL: define {{.*}}i32 @_Z11assert_loopi(
// CHECK: while.body:
// CHECK: call void @__cxa_contract_violation_assert_enforce_pf(
// CHECK-NOT: contract_violation_post
// CHECK: contract.end:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
int assert_loop(int x) post(true) { while (true) { contract_assert(x > 0); } }

// Returning on one path: the postcondition is checked on that path, against
// the returned value.
// CHECK-LABEL: define {{.*}}i32 @_Z8one_pathb(
// CHECK: store i32 1, ptr %retval
// CHECK: icmp sgt i32 %{{.*}}, 0
// CHECK: call void @__cxa_contract_violation_post_enforce_pf(
// CHECK: call void @__cxa_throw(
// CHECK: contract.end:
// CHECK: ret i32
// CHECK-NEXT: }
int one_path(bool b) post(r: r > 0) { if (b) return 1; throw 2; }

// A virtual function's own postcondition, under P3097.
#ifdef P3097
struct B { virtual int v() post(true); };
// P3097-LABEL: define {{.*}}i32 @_ZN1B1vEv(
// P3097-NOT: contract
// P3097: while.body:
// P3097-NEXT: br label %while.body
// P3097-NEXT: }
int B::v() post(true) { while (true) {} }
#endif

// A lambda's call operator, emitted after its user.
// CHECK-LABEL: define {{.*}}i32 @_Z11call_lambdav()
// CHECK-LABEL: define {{.*}}i32 @"_ZZ11call_lambdavENK3$_0clEv"(
// CHECK-NOT: contract
// CHECK: while.body:
// CHECK-NEXT: br label %while.body
// CHECK-NEXT: }
int call_lambda() {
  auto l = []() -> int post(true) { while (true) {} };
  return l();
}
