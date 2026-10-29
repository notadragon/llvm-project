// P3100: every leaf of DECISIONS.md E125 (a), configured by its own name,
// under each of the seven semantics: "assume" checks nothing and performs
// the operation (a division traps); "ignore" yields the substitute (0, or the
// wrapped result of a signed overflow) with no report; "observe" and
// "noexcept_observe" report and yield it; "enforce" and "noexcept_enforce"
// report and terminate; "quick_enforce" traps with no report.  Clang's
// signed-overflow and invalid-value checks support a throwing handler, so
// they report "observe" and "enforce" as such.  The operations are in
// p3100-ub-leaves.h.  At -O0.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ub-leaves-{assume,ignore,observe,
// enforce,quick-enforce,noexcept-observe,noexcept-enforce}.C)
//
// DEFINE: %{sem} = assume
// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// DEFINE:   -fcontracts-p4298 -O0 %libcxx_flags -o %t \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.div:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int.mod:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.div:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result.int.mod:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shl:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width.shr:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.sub:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.mul:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.negate:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool:%{sem} \
// DEFINE:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.enum:%{sem}
// RUN: %{run} && %t | FileCheck %s --check-prefix=ASSUME
// REDEFINE: %{sem} = ignore
// RUN: %{run} && %t | FileCheck %s --check-prefix=IGNORE
// REDEFINE: %{sem} = observe
// RUN: %{run} && %t | FileCheck %s --check-prefix=OBSERVE
// REDEFINE: %{sem} = enforce
// RUN: %{run} && %t | FileCheck %s --check-prefix=ENFORCE
// REDEFINE: %{sem} = quick_enforce
// RUN: %{run} && %t | FileCheck %s --check-prefix=QUICK
// REDEFINE: %{sem} = noexcept_observe
// RUN: %{run} && %t | FileCheck %s --check-prefix=NXOBS
// REDEFINE: %{sem} = noexcept_enforce
// RUN: %{run} && %t | FileCheck %s --check-prefix=NXENF

#include "p3100-ub-leaves.h"

// ASSUME: div_zero: fpe
// ASSUME-NEXT: div_zero_assign: fpe
// ASSUME-NEXT: mod_zero: fpe
// ASSUME-NEXT: div_repr: fpe
// ASSUME-NEXT: mod_repr: fpe
// ASSUME-NEXT: shl: ={{-?[0-9]+}}
// ASSUME-NEXT: shl_neg: ={{-?[0-9]+}}
// ASSUME-NEXT: shr: ={{-?[0-9]+}}
// ASSUME-NEXT: add: ={{-?[0-9]+}}
// ASSUME-NEXT: inc: ={{-?[0-9]+}}
// ASSUME-NEXT: sub: ={{-?[0-9]+}}
// ASSUME-NEXT: sub_const: ={{-?[0-9]+}}
// ASSUME-NEXT: dec: ={{-?[0-9]+}}
// ASSUME-NEXT: mul: ={{-?[0-9]+}}
// ASSUME-NEXT: mul_assign: ={{-?[0-9]+}}
// ASSUME-NEXT: negate: ={{-?[0-9]+}}
// ASSUME-NEXT: load_bool: ={{-?[0-9]+}}
// ASSUME-NEXT: load_enum: ={{-?[0-9]+}}

// IGNORE: div_zero: =0
// IGNORE-NEXT: div_zero_assign: =0
// IGNORE-NEXT: mod_zero: =0
// IGNORE-NEXT: div_repr: =0
// IGNORE-NEXT: mod_repr: =0
// IGNORE-NEXT: shl: =0
// IGNORE-NEXT: shl_neg: =0
// IGNORE-NEXT: shr: =0
// IGNORE-NEXT: add: =-2147483648
// IGNORE-NEXT: inc: =-2147483648
// IGNORE-NEXT: sub: =2147483647
// IGNORE-NEXT: sub_const: =2147483647
// IGNORE-NEXT: dec: =2147483647
// IGNORE-NEXT: mul: =-2
// IGNORE-NEXT: mul_assign: =-2
// IGNORE-NEXT: negate: =-2147483648
// IGNORE-NEXT: load_bool: =0
// IGNORE-NEXT: load_enum: =0

// OBSERVE: div_zero: R2 =0
// OBSERVE-NEXT: div_zero_assign: R2 =0
// OBSERVE-NEXT: mod_zero: R2 =0
// OBSERVE-NEXT: div_repr: R2 =0
// OBSERVE-NEXT: mod_repr: R2 =0
// OBSERVE-NEXT: shl: R2 =0
// OBSERVE-NEXT: shl_neg: R2 =0
// OBSERVE-NEXT: shr: R2 =0
// OBSERVE-NEXT: add: R2 =-2147483648
// OBSERVE-NEXT: inc: R2 =-2147483648
// OBSERVE-NEXT: sub: R2 =2147483647
// OBSERVE-NEXT: sub_const: R2 =2147483647
// OBSERVE-NEXT: dec: R2 =2147483647
// OBSERVE-NEXT: mul: R2 =-2
// OBSERVE-NEXT: mul_assign: R2 =-2
// OBSERVE-NEXT: negate: R2 =-2147483648
// OBSERVE-NEXT: load_bool: R2 =0
// OBSERVE-NEXT: load_enum: R2 =0

// ENFORCE: div_zero: R3 abort
// ENFORCE-NEXT: div_zero_assign: R3 abort
// ENFORCE-NEXT: mod_zero: R3 abort
// ENFORCE-NEXT: div_repr: R3 abort
// ENFORCE-NEXT: mod_repr: R3 abort
// ENFORCE-NEXT: shl: R3 abort
// ENFORCE-NEXT: shl_neg: R3 abort
// ENFORCE-NEXT: shr: R3 abort
// ENFORCE-NEXT: add: R3 abort
// ENFORCE-NEXT: inc: R3 abort
// ENFORCE-NEXT: sub: R3 abort
// ENFORCE-NEXT: sub_const: R3 abort
// ENFORCE-NEXT: dec: R3 abort
// ENFORCE-NEXT: mul: R3 abort
// ENFORCE-NEXT: mul_assign: R3 abort
// ENFORCE-NEXT: negate: R3 abort
// ENFORCE-NEXT: load_bool: R3 abort
// ENFORCE-NEXT: load_enum: R3 abort

// QUICK: div_zero: trap
// QUICK-NEXT: div_zero_assign: trap
// QUICK-NEXT: mod_zero: trap
// QUICK-NEXT: div_repr: trap
// QUICK-NEXT: mod_repr: trap
// QUICK-NEXT: shl: trap
// QUICK-NEXT: shl_neg: trap
// QUICK-NEXT: shr: trap
// QUICK-NEXT: add: trap
// QUICK-NEXT: inc: trap
// QUICK-NEXT: sub: trap
// QUICK-NEXT: sub_const: trap
// QUICK-NEXT: dec: trap
// QUICK-NEXT: mul: trap
// QUICK-NEXT: mul_assign: trap
// QUICK-NEXT: negate: trap
// QUICK-NEXT: load_bool: trap
// QUICK-NEXT: load_enum: trap

// NXOBS: div_zero: R6 =0
// NXOBS-NEXT: div_zero_assign: R6 =0
// NXOBS-NEXT: mod_zero: R6 =0
// NXOBS-NEXT: div_repr: R6 =0
// NXOBS-NEXT: mod_repr: R6 =0
// NXOBS-NEXT: shl: R6 =0
// NXOBS-NEXT: shl_neg: R6 =0
// NXOBS-NEXT: shr: R6 =0
// NXOBS-NEXT: add: R6 =-2147483648
// NXOBS-NEXT: inc: R6 =-2147483648
// NXOBS-NEXT: sub: R6 =2147483647
// NXOBS-NEXT: sub_const: R6 =2147483647
// NXOBS-NEXT: dec: R6 =2147483647
// NXOBS-NEXT: mul: R6 =-2
// NXOBS-NEXT: mul_assign: R6 =-2
// NXOBS-NEXT: negate: R6 =-2147483648
// NXOBS-NEXT: load_bool: R6 =0
// NXOBS-NEXT: load_enum: R6 =0

// NXENF: div_zero: R7 abort
// NXENF-NEXT: div_zero_assign: R7 abort
// NXENF-NEXT: mod_zero: R7 abort
// NXENF-NEXT: div_repr: R7 abort
// NXENF-NEXT: mod_repr: R7 abort
// NXENF-NEXT: shl: R7 abort
// NXENF-NEXT: shl_neg: R7 abort
// NXENF-NEXT: shr: R7 abort
// NXENF-NEXT: add: R7 abort
// NXENF-NEXT: inc: R7 abort
// NXENF-NEXT: sub: R7 abort
// NXENF-NEXT: sub_const: R7 abort
// NXENF-NEXT: dec: R7 abort
// NXENF-NEXT: mul: R7 abort
// NXENF-NEXT: mul_assign: R7 abort
// NXENF-NEXT: negate: R7 abort
// NXENF-NEXT: load_bool: R7 abort
// NXENF-NEXT: load_enum: R7 abort
