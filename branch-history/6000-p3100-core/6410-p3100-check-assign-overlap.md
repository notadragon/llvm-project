---
id: 6410-p3100-check-assign-overlap
subject: '[clang][contracts][P3100] implicit check: assignment from an overlapping object'
depends: [6000-p3100-core, 6110-p3100-check-null-and-alignment]
regenerates: []
fixes: []
---

## Rationale
A built-in assignment whose stored value is read from an object that
overlaps the destination, unless the overlap is exact and the two objects
have the same type -- `ub:expr.assign.overlap`, which [expr.assign] makes
undefined, for instance assigning one member of a union from a differently
typed member -- as an implicit contract assertion, with GCC's group name
and comment.

A scalar simple assignment whose right operand is, through conversions of
the value read, a glvalue is checked: `P3100AssignOverlapScope` names that
glvalue while the right operand is emitted, `EmitCheckedLValue` records the
address its load reads, and before the store the two address ranges are
compared as integers.  Any overlap between objects of different types is a
violation, and an inexact one between objects of the same type.  Neither
operand is emitted twice, so operands with side effects (an index
incremented in the left operand, a volatile source, a source reached through
a call) are checked too, which GCC does not do.  A bit-field, two different
variables (named directly, through `.` and subscripts of arrays) and a
semantic of assume or ignore emit nothing.  Both assignment emitters are
covered: `EmitBinaryOperatorLValue` and `ScalarExprEmitter::VisitBinAssign`
(an assignment whose value is used).

All seven semantics are supported; the assignment goes ahead after a
violation that returns.  Constant evaluation is untouched (the evaluator
accepts the overlapping union assignment, the stock miss CLANG-716).

A call of a constexpr function that is not manifestly constant-evaluated
keeps its run-time checks even where it could be folded (`int r =
overlap();`, a constructor initializing an automatic variable): one test
pins that for this check, the base mem-initializer call and the pure
virtual call in a constructor, at -O0 and -O2.

## Compile gap
Needs `resolveImplicitSemantic` and `emitImplicitGuardReaction` from
`6000-p3100-core`; both precede it.  The hooks are in `CGExpr.cpp` and
`CGExprScalar.cpp`, which `6110-p3100-check-null-and-alignment` and
`6000-p3100-core` claim whole, and the declarations share earlier hunks of
`CodeGenFunction.h`, so this commit carries the scope, its helpers and the
tests.

## Contents
- clang/lib/CodeGen/CGContracts.cpp : @stripP3100OverlapGlvalue, @getP3100OverlapBaseVar, @isP3100OverlapValueConversion, @P3100AssignOverlapScope, @check
- clang/test/Contracts/Runnable/p3100-assign-overlap-assume.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-edges.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-noexcept-enforce.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-noexcept-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-observe.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-quick.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-side-effects.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-side-effects-order.cpp : *
- clang/test/Contracts/Runnable/p3100-assign-overlap-throw.cpp : *
- clang/test/Contracts/Runnable/p3100-constexpr-fold-runtime-check.cpp : *
- clang/test/Contracts/p3100-assign-overlap-constexpr.cpp : *
- clang/test/Contracts/p3100-assign-overlap-side-effects-constexpr.cpp : *
