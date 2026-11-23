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

Some passing files pin core-language constant-evaluation behaviour, with no
contracts involved, on programs that one of the two compilers gets wrong
(`constexpr-double-destroy.cpp` and the like), so the mirror of a GCC watch
test lives here too.  One pins a correct rejection that looks like a bug:
copying a padding byte from a `std::bit_cast` result into an `unsigned char`
is well-defined at run time, but [expr.const] excludes an lvalue-to-rvalue
conversion of an indeterminate value from constant evaluation
(`constexpr-indeterminate-uchar-copy.cpp`).

The stock constant-evaluation bugs the D4277R1 per-UB review filed have
watch tests in `OpenBugs/` (CLANG-688, CLANG-689, CLANG-710..CLANG-718,
CLANG-725, CLANG-737), and the programs of the GCC fork's watch tests for
its own rows have mirrors here: a passing test where Clang gets the program
right, an `OpenBugs/` watch where it shares the bug.  The watches use
`-verify-ignore-unexpected=note`, so that the fix's notes do not keep the
file failing and hide the XPASS.

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
- clang/test/Contracts/OpenBugs/vptr-base-member.cpp : *
- clang/test/Contracts/OpenBugs/ubsan-null-base-member.cpp : *
- clang/test/Contracts/constexpr-member-before-construction.cpp : *
- clang/test/Contracts/constexpr-indeterminate-uchar-copy.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-address-past-end-indirection.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-call-in-base-mem-init.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-cdtor-convert-virtual-base.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-cdtor-dynamic-cast.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-in-base-mem-init.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-vbase-construction.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-enum-floating-cast.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-noreturn-returns.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-null-static-member-call.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-null-function-indirection.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-past-end-nonvirtual-base.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-reference-through-dangling-pointer.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-reference-to-freed-storage.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-union-overlap-assignment.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-unsequenced-modification.cpp : *
- clang/test/Contracts/OpenBugs/vptr-downcast-during-construction.cpp : *
- clang/test/Contracts/OpenBugs/vptr-reinterpret-reference.cpp : *
- clang/test/Contracts/OpenBugs/p3100-routed-handler-effects-late.cpp : *
- clang/test/Contracts/OpenBugs/ubsan-null-trivial-assign-call.cpp : *
- clang/test/Contracts/OpenBugs/constexpr-empty-copy-past-end.cpp : *
- clang/test/Contracts/OpenBugs/ubsan-null-downcast-ref-offset.cpp : *
- clang/test/Contracts/OpenBugs/ubsan-alignment-char-array.cpp : *
- clang/test/Contracts/OpenBugs/ubsan-null-dynamic-cast-reference.cpp : *
- clang/test/Contracts/constexpr-array-delete-via-base.cpp : *
- clang/test/Contracts/constexpr-create-in-const-object.cpp : *
- clang/test/Contracts/constexpr-delete-allocator-storage.cpp : *
- clang/test/Contracts/constexpr-double-destruction-automatic.cpp : *
- clang/test/Contracts/constexpr-dynamic-cast-past-end-downcast.cpp : *
- clang/test/Contracts/constexpr-dynamic-cast-typeid-outside-lifetime.cpp : *
- clang/test/Contracts/constexpr-empty-store-past-end.cpp : *
- clang/test/Contracts/constexpr-enum-out-of-range.cpp : *
- clang/test/Contracts/constexpr-function-try-block-handler-member.cpp : *
- clang/test/Contracts/constexpr-heap-after-dtor.cpp : *
- clang/test/Contracts/constexpr-member-call-outside-lifetime.cpp : *
- clang/test/Contracts/constexpr-modify-new-const.cpp : *
- clang/test/Contracts/constexpr-null-data-member-pointer-address.cpp : *
- clang/test/Contracts/constexpr-null-indirection-no-read.cpp : *
- clang/test/Contracts/constexpr-oob-pointer-formation.cpp : *
- clang/test/Contracts/constexpr-ptrmem-cast-to-base.cpp : *
- clang/test/Contracts/constexpr-ptrmem-cast-unrelated-base.cpp : *
- clang/test/Contracts/constexpr-ptrmem-object-lacks-member.cpp : *
- clang/test/Contracts/constexpr-pure-virtual-direct-call.cpp : *
- clang/test/Contracts/constexpr-reference-reinterpret-cast.cpp : *
- clang/test/Contracts/constexpr-reference-self-init.cpp : *
- clang/test/Contracts/constexpr-reference-through-invalid-pointer.cpp : *
- clang/test/Contracts/constexpr-sub-same-object.cpp : *
- clang/test/Contracts/constexpr-vbase-conversion-destroyed-named.cpp : *
- clang/test/Contracts/constexpr-vbase-dtor-non-constexpr.cpp : *
- clang/test/Contracts/constexpr-virtual-call-base-mem-init.cpp : *
- clang/test/Contracts/constexpr-void-cast-pointer-plus.cpp : *
- clang/test/Contracts/constexpr-volatile-through-nonvolatile.cpp : *
