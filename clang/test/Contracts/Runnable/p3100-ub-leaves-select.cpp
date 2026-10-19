// P3100: each implicit assertion of DECISIONS.md E125 (a) reports its leaf,
// which is configured on its own.  SELA observes (noexcept_observe) one leaf
// of each group -- ub:expr.mul.div.by.zero.int.div,
// ub:expr.mul.representable.type.result.int.mod,
// ub:expr.shift.neg.and.width.shl, ub:expr.expr.eval.signed.integer.add and
// .mul, ub:conv.lval.valid.representation.bool -- and the others keep the
// default "assume"; SELB swaps them.  At -O0 and -O2.  The operations are in
// p3100-ub-leaves.h.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ub-leaves-select-{a,b}{,-O2}.C)
//
// DEFINE: %{opt} = -O0
// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// DEFINE:   -fcontracts-p4298 %{opt} %libcxx_flags -o %t
// RUN: %{run} \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:noexcept_observe
// RUN: %t | FileCheck %s --check-prefix=SELA
// RUN: %{run} \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:noexcept_observe
// RUN: %t | FileCheck %s --check-prefix=SELB
// REDEFINE: %{opt} = -O2
// RUN: %{run} \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:noexcept_observe
// RUN: %t | FileCheck %s --check-prefix=SELA
// RUN: %{run} \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:noexcept_observe
// RUN: %t | FileCheck %s --check-prefix=SELB

#include "p3100-ub-leaves.h"

// SELA: div_zero: R6 =0
// SELA-NEXT: div_zero_assign: R6 =0
// SELA-NEXT: mod_zero: fpe
// SELA-NEXT: div_repr: fpe
// SELA-NEXT: mod_repr: R6 =0
// SELA-NEXT: shl: R6 =0
// SELA-NEXT: shl_neg: R6 =0
// SELA-NEXT: shr: ={{-?[0-9]+}}
// SELA-NEXT: add: R6 =-2147483648
// SELA-NEXT: inc: R6 =-2147483648
// SELA-NEXT: sub: ={{-?[0-9]+}}
// SELA-NEXT: sub_const: ={{-?[0-9]+}}
// SELA-NEXT: dec: ={{-?[0-9]+}}
// SELA-NEXT: mul: R6 =-2
// SELA-NEXT: mul_assign: R6 =-2
// SELA-NEXT: negate: ={{-?[0-9]+}}
// SELA-NEXT: load_bool: R6 =0
// SELA-NEXT: load_enum: ={{-?[0-9]+}}

// SELB: div_zero: fpe
// SELB-NEXT: div_zero_assign: fpe
// SELB-NEXT: mod_zero: R6 =0
// SELB-NEXT: div_repr: R6 =0
// SELB-NEXT: mod_repr: fpe
// SELB-NEXT: shl: ={{-?[0-9]+}}
// SELB-NEXT: shl_neg: ={{-?[0-9]+}}
// SELB-NEXT: shr: R6 =0
// SELB-NEXT: add: ={{-?[0-9]+}}
// SELB-NEXT: inc: ={{-?[0-9]+}}
// SELB-NEXT: sub: R6 =2147483647
// SELB-NEXT: sub_const: R6 =2147483647
// SELB-NEXT: dec: R6 =2147483647
// SELB-NEXT: mul: ={{-?[0-9]+}}
// SELB-NEXT: mul_assign: ={{-?[0-9]+}}
// SELB-NEXT: negate: R6 =-2147483648
// SELB-NEXT: load_bool: ={{-?[0-9]+}}
// SELB-NEXT: load_enum: R6 =0
