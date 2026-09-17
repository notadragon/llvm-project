---
id: 1190-base-open-bug-watch
subject: '[clang][contracts] watch tests for the bugs still open here'
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

One is the opposite and is not xfailed, because the defect it watches is
GCC's: `postcondition-undeduced-result.cpp` pins behaviour that is correct
here and wrong there, so a regression on this side is caught while the mirror
on that side stays xfailed.  Same test content, per-compiler expectation.

They come after `1180-p2900-base-tests` because several exercise the base
facility end to end and need the same harness.


## Compile gap
None.  Every one is a test file; nothing in the compiler depends on them.

## Contents
- clang/test/Contracts/OpenBugs/address-before-ctor-member.cpp : *
- clang/test/Contracts/OpenBugs/address-before-ctor-vbase.cpp : *
- clang/test/Contracts/OpenBugs/deferred-friend-contract-mismatch.cpp : *
- clang/test/Contracts/OpenBugs/postcondition-undeduced-result.cpp : *
- clang/test/Contracts/OpenBugs/retval-throwing-cleanup.cpp : *
- clang/test/Contracts/OpenBugs/this-in-xobj-noexcept-specifier.cpp : *
- clang/test/Contracts/OpenBugs/this-in-xobj-trailing-return-type.cpp : *
