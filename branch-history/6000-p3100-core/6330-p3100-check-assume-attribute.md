---
id: 6330-p3100-check-assume-attribute
subject: '[clang][contracts] implicit check: the [[assume]] attribute'
depends: [6000-p3100-core]
regenerates: []
fixes: []
---

## Rationale
`[[assume]]` under P3100: the attribute becomes a contract assertion whose
semantic is configurable, so an assumption can be *checked* rather than
believed.

This is the band's most consequential entry and the only one with a flag of
its own.  `-fcontracts-allow-assume` (default **off**) is what permits a
resolved `assume` semantic to be used at all; without it a resolved
`assume` is treated as `ignore`.  The distinction matters because `assume`
is the one semantic that lets the optimiser act on an unverified claim, and
the default is that we do not.

`EmitCXXAssumeCheck` is the checking form and `EmitCXXAssumeAttr` the
replacement for the attribute's ordinary emission; `CGStmt.cpp` is where
the two are selected between.  `StmtCXX.cpp`'s addition is the AST side of
carrying the resolved semantic on the statement.

`__clang_contracts_allow_assume` is predefined when the flag is on -- a
vendor macro rather than a standard feature-test macro, because there is no
paper that defines one.

**The gate applies to hand-written contracts only.**  An implicit or
routed contract assertion may use `assume` without it: the flag exists so
that a human writing `assume` in source has to opt in, not to second-guess
a check the compiler emitted itself.

## Compile gap
Needs `6000-p3100-core` for semantic resolution and
`1100-p2900-base-codegen` for the emission path; both precede it.

None outward.

**Behaviour trap, and it is the worst in the band.**  With the flag off, a
contract configured to `assume` silently becomes `ignore` -- the predicate
is not evaluated and the optimiser is told nothing.  That is the safe
direction.  The unsafe direction is the same misconfiguration with the flag
*on* and the predicate false: the optimiser proceeds from a false premise,
and there is no diagnostic at any point.  Which is the whole reason the
default is off.

## Contents

- clang/include/clang/Options/Options.td : @assume
- clang/lib/AST/StmtCXX.cpp : @applyAssumeGate
- clang/lib/CodeGen/CGContracts.cpp : @EmitCXXAssumeCheck, @EmitCXXAssumeAttr
- clang/lib/CodeGen/CGStmt.cpp : #2
- clang/lib/CodeGen/CodeGenFunction.cpp : #4
- clang/lib/Frontend/InitPreprocessor.cpp : #0:9
- clang/test/Contracts/Runnable/p3100-assume-allowed.cpp : *
- clang/test/Contracts/Runnable/p3100-assume-ignore.cpp : *
- clang/test/Contracts/Runnable/p3100-assume-nonpure-p4298.cpp : *
- clang/test/Contracts/Runnable/p3100-assume-nonpure-p4298.json : *
- clang/test/Contracts/p3100-allow-assume-ftm.cpp : *
- clang/test/Contracts/p3100-allow-assume-no-flag.cpp : *
- clang/test/Contracts/p3100-assume-attr-codegen.cpp : *
- clang/test/Contracts/p3100-assume-attr-codegen.json : *
- clang/test/Contracts/p3100-assume-attr-enforce-predicates.cpp : *
- clang/test/Contracts/p3100-assume-attr-enforce-predicates.json : *
- clang/test/Contracts/p3100-assume-attr-predicates.cpp : *
- clang/test/Contracts/p3100-assume-attr-predicates.json : *
- clang/test/Contracts/p3100-assume-attr-pure-group.cpp : *
- clang/test/Contracts/p3100-assume-attr-pure-group.json : *
- clang/test/Contracts/p3100-assume-attr-pure.cpp : *
- clang/test/Contracts/p3100-assume-attr-pure.json : *
- clang/test/Contracts/p3100-assume-attr-quick.cpp : *
- clang/test/Contracts/p3100-assume-attr-quick.json : *
- clang/test/Contracts/p3100-assume-attr-throw-noexcept.cpp : *
- clang/test/Contracts/p3100-assume-attr-throw-noexcept.json : *
- clang/test/Contracts/p3100-assume-attr-throw.cpp : *
- clang/test/Contracts/p3100-assume-attr-throw.json : *
- clang/test/Contracts/p3100-assume-attr.cpp : *
- clang/test/Contracts/p3100-assume-attr.json : *
