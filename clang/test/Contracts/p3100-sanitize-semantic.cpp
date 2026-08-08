// P3100: -fsanitize-semantic= driver flag + routed-sanitizer semantic model.
// These are driver diagnostics / driver-resolved cc1 rendering, so they are
// checked with -### and FileCheck rather than -verify.

// -- Valid explicit requests -------------------------------------------------

// address:noexcept_enforce is valid with -fcontracts-p4298.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=address:noexcept_enforce"

// address:quick_enforce is valid in any build (no p4298 needed).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:quick_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=OK-QE
// OK-QE-NOT: error:
// OK-QE: "-fsanitize-semantic=address:quick_enforce"

// -- Routed-check throwing semantics are hard errors -------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=address:enforce' is not supported for a routed sanitizer check
// ERR-ENFORCE-SAME: noexcept_enforce
// ERR-ENFORCE-SAME: quick_enforce

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address \
// RUN:   -fsanitize-semantic=address:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-OBSERVE
// ERR-OBSERVE: error: '-fsanitize-semantic=address:observe' is not supported for a routed sanitizer check
// ERR-OBSERVE-SAME: noexcept_observe

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:ignore %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-IGNORE
// ERR-IGNORE: error: '-fsanitize-semantic=address:ignore' is not supported

// -- p4298 gate --------------------------------------------------------------

// address:noexcept_enforce WITHOUT -fcontracts-p4298 -> requires the flag.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=ERR-NXE-NOP4298
// ERR-NXE-NOP4298: error: '-fsanitize-semantic=address:noexcept_enforce' requires '-fcontracts-p4298'

// -fsanitize-recover=address (derived noexcept_observe) WITHOUT p4298 -> error.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-recover=address %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-RECOVER-NOP4298
// ERR-RECOVER-NOP4298: error: '-fsanitize-recover=address' routing to the contract-violation handler requires '-fcontracts-p4298'

// -- p4298 gate is skipped under the -fsanitize-noncontract-callbacks opt-out --
// (routing is off, so no report is dispatched through the handler and there is
// nothing to gate).  Mirrors GCC's gate exactly (gcc/opts.cc: the routed-
// semantic block is guarded on !flag_sanitize_noncontract_callbacks).

// address:noexcept_enforce WITHOUT p4298 but WITH the opt-out -> compiles clean.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce \
// RUN:   -fsanitize-noncontract-callbacks %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OPTOUT-NXE-NOP4298
// OPTOUT-NXE-NOP4298-NOT: error:

// -fsanitize-recover=address (derived noexcept_observe) WITHOUT p4298 but WITH
// the opt-out -> compiles clean (the case the coordinator flagged; GCC accepts
// it too).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-recover=address \
// RUN:   -fsanitize-noncontract-callbacks %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OPTOUT-RECOVER-NOP4298
// OPTOUT-RECOVER-NOP4298-NOT: error:

// -- ASan pointer-pair checks are routed ------------------------------------

// pointer-compare:quick_enforce is valid (routed; quick_enforce always
// available).  pointer-compare must be combined with -fsanitize=address.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize=pointer-compare \
// RUN:   -fsanitize-semantic=pointer-compare:quick_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-PCMP-QE
// OK-PCMP-QE-NOT: error:
// OK-PCMP-QE: "-fsanitize-semantic=pointer-compare:quick_enforce"

// pointer-compare:observe (plain throwing) is a hard error for the routed check.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address -fsanitize=pointer-compare \
// RUN:   -fsanitize-semantic=pointer-compare:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-PCMP-OBSERVE
// ERR-PCMP-OBSERVE: error: '-fsanitize-semantic=pointer-compare:observe' is not supported for a routed sanitizer check
// ERR-PCMP-OBSERVE-SAME: noexcept_observe

// pointer-subtract:noexcept_observe is valid under -fcontracts-p4298.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=address -fsanitize=pointer-subtract \
// RUN:   -fsanitize-semantic=pointer-subtract:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-PSUB-NXO
// OK-PSUB-NXO-NOT: error:
// OK-PSUB-NXO: "-fsanitize-semantic=pointer-subtract:noexcept_observe"

// -- Non-routed individual checks -------------------------------------------

// return:observe (return cannot recover) -> hard error.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=return -fsanitize-semantic=return:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-RETURN-OBSERVE
// ERR-RETURN-OBSERVE: error: '-fsanitize-semantic=return:observe' is not supported

// kernel-address:quick_enforce (kernel-address cannot trap and is not routed)
// -> hard error naming kernel-address.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=kernel-address \
// RUN:   -fsanitize-semantic=kernel-address:quick_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-KASAN-QE
// ERR-KASAN-QE: error: '-fsanitize-semantic=kernel-address:quick_enforce' is not supported

// -- Bad names --------------------------------------------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize-semantic=bogus:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-BADCHECK
// ERR-BADCHECK: error: '-fsanitize-semantic=' unknown sanitizer check 'bogus'

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:bogus %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-BADSEM
// ERR-BADSEM: error: '-fsanitize-semantic=' unknown contract evaluation semantic 'bogus'

// -- Group behavior ---------------------------------------------------------

// undefined:noexcept_observe: shift and return are now routed UBSan checks, and
// the routed allowed set includes noexcept_observe uniformly (return is
// non-recoverable but still accepts it -- its noreturn report leg just
// terminates after the handler), so every group member takes it.  Plain
// "observe" is a hard error for these routed members; the routed continuing
// semantic is noexcept_observe (needs -fcontracts-p4298).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=shift,return \
// RUN:   -fsanitize-semantic=undefined:noexcept_observe %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=GROUP-OBSERVE
// GROUP-OBSERVE-NOT: error:
// GROUP-OBSERVE-DAG: "-fsanitize-semantic=shift-base:noexcept_observe"
// GROUP-OBSERVE-DAG: "-fsanitize-semantic=return:noexcept_observe"

// undefined:ignore -> hard error even via a group name.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=undefined -fsanitize-semantic=undefined:ignore %s -### \
// RUN:   2>&1 | FileCheck %s --check-prefix=GROUP-IGNORE
// GROUP-IGNORE: error: '-fsanitize-semantic=undefined:ignore' is not supported

// -- "all" meta-group must not admit a throwing semantic on the routed
//    address member (regression: routed-ness must be tested PER EXPANDED BIT,
//    not on the "all" group bit, which does not itself contain the address
//    bit).  Matches GCC: a meta-group silently restricts to capable members,
//    so the routed address member falls back to its derived value
//    (quick_enforce, no p4298) rather than storing a throwing observe/enforce.

// all:observe -> NO error, and address must NOT be rendered as observe; it is
// silently restricted to its derived quick_enforce.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=all:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ALL-OBSERVE
// ALL-OBSERVE-NOT: error:
// ALL-OBSERVE-NOT: "-fsanitize-semantic=address:observe"
// ALL-OBSERVE: "-fsanitize-semantic=address:quick_enforce"

// all:enforce -> same: address stays derived quick_enforce, no throwing enforce.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=all:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ALL-ENFORCE
// ALL-ENFORCE-NOT: error:
// ALL-ENFORCE-NOT: "-fsanitize-semantic=address:enforce"
// ALL-ENFORCE: "-fsanitize-semantic=address:quick_enforce"

// all:noexcept_observe WITHOUT p4298 -> the address member stores
// noexcept_observe (routed set) and the p4298 gate then hard-errors.
// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=all:noexcept_observe %s -### \
// RUN:   2>&1 | FileCheck %s --check-prefix=ALL-NXO-NOP4298
// ALL-NXO-NOP4298: error: '-fsanitize-semantic=address:noexcept_observe' requires '-fcontracts-p4298'

// all:quick_enforce -> OK for the routed address member.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=all:quick_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ALL-QE
// ALL-QE-NOT: error:
// ALL-QE: "-fsanitize-semantic=address:quick_enforce"

// -- Derivation observability (print seam) ----------------------------------

// default -fsanitize=address with p4298 derives noexcept_enforce.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce \
// RUN:   -fsanitize-semantic-print -emit-obj -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PRINT-NXE
// PRINT-NXE: address: noexcept_enforce

// default -fsanitize=address without p4298 derives quick_enforce (driver
// resolves it and renders it to cc1).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DERIVE-DEFAULT
// DERIVE-DEFAULT: "-fsanitize-semantic=address:quick_enforce"

int main() { return 0; }
