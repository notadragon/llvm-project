// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-obj -o %t.o %s
// RUN: llvm-nm %t.o | FileCheck --check-prefix=NM %s
// REQUIRES: x86-registered-target

// P4405 stage 3: neither a proof shadow nor anything only it uses is
// emitted.  An inline function and a static function called only from
// predicates, and not inlined into the shadow, keep no definition in the
// object file; nor does any shadow, nor the shadow's markers; a function
// the real code calls is still emitted.  The second function's evaluation
// is proven soundly, so -fcontracts-prove-assume (implied) keeps an
// assumption made of it, through a predicate-only function too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-symbols.C

[[gnu::noinline]] inline bool only_in_predicate(int x) { return x * 3 != 7; }
[[gnu::noinline]] static bool static_in_predicate(int x) { return x * 5 != 9; }
[[gnu::noinline]] inline int used_by_code(int x) { return x * 7; }
inline bool positive(int x) { return x > 0; }

int f(int x) {
  contract_assert(only_in_predicate(x));
  contract_assert(static_in_predicate(x));
  return used_by_code(x);
}

int g(int x) {
  if (x > 0)
    contract_assert(x > 0 || only_in_predicate(x));
  contract_assert(positive(x) || x <= 0);
  return x;
}

// CHECK-NOT: only_in_predicate
// CHECK-NOT: static_in_predicate
// CHECK-NOT: positive
// CHECK-NOT: contract.proof
// CHECK-NOT: __clang_contract
// CHECK-NOT: clang.contract
// CHECK: define {{.*}} @_Z1fi(
// CHECK-NOT: only_in_predicate
// CHECK-NOT: static_in_predicate
// CHECK-NOT: positive
// CHECK-NOT: contract.proof
// CHECK-NOT: __clang_contract
// CHECK-NOT: clang.contract
// CHECK: define linkonce_odr {{.*}} @_Z12used_by_codei(
// CHECK-NOT: only_in_predicate
// CHECK-NOT: static_in_predicate
// CHECK-NOT: positive
// CHECK-NOT: contract.proof
// CHECK-NOT: __clang_contract
// CHECK-NOT: clang.contract

// NM-NOT: only_in_predicate
// NM-NOT: static_in_predicate
// NM-NOT: positive
// NM-NOT: contract
// NM: _Z12used_by_codei
// NM-NOT: only_in_predicate
// NM-NOT: static_in_predicate
// NM-NOT: positive
// NM-NOT: contract
