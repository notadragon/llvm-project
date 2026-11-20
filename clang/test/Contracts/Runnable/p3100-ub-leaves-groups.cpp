// P3100: the names the checks of DECISIONS.md E125 (a) reported before their
// leaves stay accepted, as groups selecting the leaves (GROUPS):
// ub:expr.mul.div.by.zero.int (.div, .mod),
// ub:expr.mul.representable.type.result (.int.div, .int.mod),
// ub:expr.shift.neg.and.width (.shl, .shr) and
// ub:expr.expr.eval.signed.integer (.add, .sub, .mul, .negate) by dotted
// prefix, and ub:conv.lval.valid.representation.bool.enum, the combined group,
// selects .bool and .enum, neither within its name.
// The CONFIG run: a configuration file naming a leaf and, after it, the
// group that contains it; the first entry that matches decides.
// ub:conv.lval.valid.representation.enum is ignored before the combined group
// observes .bool; ub:expr.mul.div.by.zero.int.mod is ignored before
// ub:expr.mul.div.by.zero.int observes .int.div;
// ub:expr.expr.eval.signed.integer.negate is ignored before ub:expr.expr.eval
// observes .add, .sub and .mul.  The other leaves keep the default "assume".
// The operations are in p3100-ub-leaves.h.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ub-leaves-groups{,-config}.C)
//
// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// DEFINE:   -fcontracts-p4298 -O0 %libcxx_flags -o %t
// RUN: %{run} \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero.int:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.representable.type.result:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.shift.neg.and.width:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:conv.lval.valid.representation.bool.enum:noexcept_observe
// RUN: %t | FileCheck %s --check-prefix=GROUPS
// RUN: %{run} -fcontract-configuration-file=%S/p3100-ub-leaves-groups.json
// RUN: %t | FileCheck %s --check-prefix=CONFIG

#include "p3100-ub-leaves.h"

// GROUPS: div_zero: R6 =0
// GROUPS-NEXT: div_zero_assign: R6 =0
// GROUPS-NEXT: mod_zero: R6 =0
// GROUPS-NEXT: div_repr: R6 =0
// GROUPS-NEXT: mod_repr: R6 =0
// GROUPS-NEXT: shl: R6 =0
// GROUPS-NEXT: shl_neg: R6 =0
// GROUPS-NEXT: shr: R6 =0
// GROUPS-NEXT: add: R6 =-2147483648
// GROUPS-NEXT: inc: R6 =-2147483648
// GROUPS-NEXT: sub: R6 =2147483647
// GROUPS-NEXT: sub_const: R6 =2147483647
// GROUPS-NEXT: dec: R6 =2147483647
// GROUPS-NEXT: mul: R6 =-2
// GROUPS-NEXT: mul_assign: R6 =-2
// GROUPS-NEXT: negate: R6 =-2147483648
// GROUPS-NEXT: load_bool: R6 =0
// GROUPS-NEXT: load_enum: R6 =0

// CONFIG: div_zero: R6 =0
// CONFIG-NEXT: div_zero_assign: R6 =0
// CONFIG-NEXT: mod_zero: =0
// CONFIG-NEXT: div_repr: fpe
// CONFIG-NEXT: mod_repr: fpe
// CONFIG-NEXT: shl: ={{-?[0-9]+}}
// CONFIG-NEXT: shl_neg: ={{-?[0-9]+}}
// CONFIG-NEXT: shr: ={{-?[0-9]+}}
// CONFIG-NEXT: add: R6 =-2147483648
// CONFIG-NEXT: inc: R6 =-2147483648
// CONFIG-NEXT: sub: R6 =2147483647
// CONFIG-NEXT: sub_const: R6 =2147483647
// CONFIG-NEXT: dec: R6 =2147483647
// CONFIG-NEXT: mul: R6 =-2
// CONFIG-NEXT: mul_assign: R6 =-2
// CONFIG-NEXT: negate: =-2147483648
// CONFIG-NEXT: load_bool: R6 =0
// CONFIG-NEXT: load_enum: =0
