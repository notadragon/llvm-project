// P4405: P3100 implicit contract assertions are unaffected by prove and
// diagnose: neither is among the semantics an implicit assertion allows, so
// the best fit takes a configured prove or diagnose to ignore -- no
// diagnostic, no report, and each operation yields its defined substitute,
// exactly as p3100-ub-leaves.cpp's IGNORE run.  The operations are in
// p3100-ub-leaves.h.  At -O0.
// (GCC mirror: g++.dg/contracts/cpp26/p4405-p3100-unaffected.C)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4405 \
// RUN:   -O0 %libcxx_flags -o %t -Xclang -verify \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:diagnose \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:diagnose \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:diagnose \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:diagnose \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:diagnose \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:prove \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:diagnose
// RUN: %t | FileCheck %s
// expected-no-diagnostics

#include "p3100-ub-leaves.h"

// CHECK: div_zero: =0
// CHECK-NEXT: div_zero_assign: =0
// CHECK-NEXT: mod_zero: =0
// CHECK-NEXT: div_repr: =0
// CHECK-NEXT: mod_repr: =0
// CHECK-NEXT: shl: =0
// CHECK-NEXT: shl_neg: =0
// CHECK-NEXT: shr: =0
// CHECK-NEXT: add: =-2147483648
// CHECK-NEXT: inc: =-2147483648
// CHECK-NEXT: sub: =2147483647
// CHECK-NEXT: sub_const: =2147483647
// CHECK-NEXT: dec: =2147483647
// CHECK-NEXT: mul: =-2
// CHECK-NEXT: mul_assign: =-2
// CHECK-NEXT: negate: =-2147483648
// CHECK-NEXT: load_bool: =0
// CHECK-NEXT: load_enum: =0
