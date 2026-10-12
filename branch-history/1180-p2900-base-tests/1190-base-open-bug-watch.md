---
id: 1190-base-open-bug-watch
subject: '[clang][contracts][P2900] watch tests for the bugs still open here'
depends: [1180-p2900-base-tests]
regenerates: []
fixes: []
---

## Rationale
Watch tests for defects that are still open -- on this branch, or on the GCC
fork this one is mirrored against -- kept together so that dropping them is
one decision rather than seven.

Most are an `XFAIL` pinning behaviour we know to be wrong: the test fails
today, and the day it starts passing the suite says so.  They are
deliberately not filed against the features they touch, because they do not
describe those features -- they describe what is broken.  `open-issues/`
carries the writeups.

Two passing files ride along: the controls for three of the watches
(`this-in-member-declaration-controls.cpp`,
`contract-redecl-mismatch-namespace-control.cpp`).  lit marks a whole file
`XFAIL`, so a control inside a watch file would regress unseen; each sits
next to the watch it anchors instead.

They come after `1180-p2900-base-tests` because several exercise the base
facility end to end and need the same harness.


## Compile gap
None.  Every one is a test file; nothing in the compiler depends on them.

## Contents
- clang/test/Contracts/OpenBugs/address-before-ctor-member.cpp : *
- clang/test/Contracts/OpenBugs/bytecode-constexpr-contract-problems.cpp : *
- clang/test/Contracts/contract-redecl-mismatch-namespace-control.cpp : *
- clang/test/Contracts/OpenBugs/retval-throwing-cleanup.cpp : *
- clang/test/Contracts/OpenBugs/this-in-xobj-noexcept-specifier.cpp : *
- clang/test/Contracts/OpenBugs/this-in-xobj-trailing-return-type.cpp : *
- clang/test/Contracts/this-in-member-declaration-controls.cpp : *
- clang/test/Contracts/constexpr-delete-nonvirtual-dtor.cpp : *
- clang/test/Contracts/constexpr-double-destroy.cpp : *
- clang/test/Contracts/stmt-expr-local-lambda-pre.cpp : *
- clang/test/Contracts/generic-lambda-pre-capture.cpp : *
- clang/test/Contracts/lambda-pre-fold-capture.cpp : *
- clang/test/Contracts/precondition-const-local-init.cpp : *
- clang/test/Contracts/constexpr-contract-observe-not-constant-init.cpp : *
- clang/test/Contracts/OpenBugs/param-cv-out-of-class-member-definition.cpp : *
- clang/test/Contracts/constexpr-member-before-construction.cpp : *
