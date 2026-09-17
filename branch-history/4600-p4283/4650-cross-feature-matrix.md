---
id: 4650-cross-feature-matrix
subject: '[clang][contracts] cross-product matrices and reentrancy tests'
depends: [3000-p3097, 4000-p3098, 4200-p3400-core, 4600-p4283]
regenerates: []
fixes: []
---

## Rationale
The tests that exercise several features at once, kept together and placed
after the last feature any of them needs.

A test naming one paper travels with that paper.  These name three or four,
so no paper's commit can carry them without pulling in the others -- which
is the rule in `phase2-design.md` 3.2 about a test that exercises several
features becoming its own commit.  It mirrors GCC's
`4650-reentrancy-everything` at the same band number.

Two families, and each is a deliberate cross-product rather than a
collection:

* **`contract-reentrancy-*`** -- a violation handler that itself triggers a
  contract, under each combination of P3097, P3098 and the umbrella.  The
  question is whether the second violation is reported, dropped, or
  recursed into forever, and the answer differs by feature combination.
* **`matrix-*`** -- exception specifications, SFINAE, and caller-side
  readers, each crossed with the papers that change the answer.
  `matrix-readers-p3097` and its `-codegen` twin are the largest at 50
  segments each.

The `-codegen` variants are not duplicates.  Each pairs a `-fsyntax-only`
run with one that actually emits and runs, because several of these
questions have a Sema answer and a CodeGen answer that have disagreed
before.

## A note on cost

3,389 test lines in one commit is a lot to ask a reviewer to read at once.
It is still the right unit: split by paper, each piece would sit at a
commit where the other papers it exercises do not yet exist, and would fail
there.

## Compile gap
Depends on `3000-p3097`, `4000-p3098`, `4200-p3400-core` and
`4600-p4283` -- the last of which is why it sits at 4650.

Nothing to stub: every file is a test.

**Behaviour trap:** none forward, but one backwards worth naming.  Several
of these tests pass vacuously if a feature they exercise is not enabled --
a reentrancy test under a flag combination that turns the inner contract
off checks nothing and reports success.  The RUN lines carry the flags
explicitly for that reason; do not "simplify" them onto the umbrella.

## Contents

- clang/test/Contracts/Runnable/contract-reentrancy.cpp : *
- clang/test/Contracts/contract-reentrancy-basic.cpp : *
- clang/test/Contracts/contract-reentrancy-p3097-p3098.cpp : *
- clang/test/Contracts/contract-reentrancy-p3097.cpp : *
- clang/test/Contracts/contract-reentrancy-p3098.cpp : *
- clang/test/Contracts/contract-reentrancy-p3850.cpp : *
- clang/test/Contracts/matrix-exceptspec-errors.cpp : *
- clang/test/Contracts/matrix-exceptspec-p3097.cpp : *
- clang/test/Contracts/matrix-exceptspec-p3098.cpp : *
- clang/test/Contracts/matrix-exceptspec-p3400.cpp : *
- clang/test/Contracts/matrix-exceptspec.cpp : *
- clang/test/Contracts/matrix-readers-caller-codegen.cpp : *
- clang/test/Contracts/matrix-readers-caller-codegen.json : *
- clang/test/Contracts/matrix-readers-caller.cpp : *
- clang/test/Contracts/matrix-readers-caller.json : *
- clang/test/Contracts/matrix-readers-codegen.cpp : *
- clang/test/Contracts/matrix-readers-p3097-codegen.cpp : *
- clang/test/Contracts/matrix-readers-p3097.cpp : *
- clang/test/Contracts/matrix-readers.cpp : *
- clang/test/Contracts/matrix-sfinae-codegen-errors.cpp : *
- clang/test/Contracts/matrix-sfinae-errors.cpp : *
- clang/test/Contracts/matrix-sfinae-p4283-errors.cpp : *
- clang/test/Contracts/matrix-sfinae-p4283-run.cpp : *
- clang/test/Contracts/matrix-sfinae-p4283.cpp : *
- clang/test/Contracts/matrix-sfinae.cpp : *
- clang/test/Contracts/matrix-throwpaths-p3850-run.cpp : *
