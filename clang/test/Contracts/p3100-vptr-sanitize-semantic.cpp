// P3100 Task 4.1: -fsanitize-semantic= for the routed UBSan vptr check --
// driver diagnostics, the recover-vs-abort code-path selection, and semantic
// print.  Driver-level, so checked with -### / FileCheck.

// -- Valid explicit requests -------------------------------------------------

// vptr:noexcept_enforce is valid with -fcontracts-p4298.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-semantic=vptr:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=vptr:noexcept_enforce"

// vptr:quick_enforce is valid in any build (no p4298 needed) even though native
// vptr has no trap mode -- quick_enforce is realized by the routing runtime.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:quick_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=OK-QE
// OK-QE-NOT: error:
// OK-QE: "-fsanitize-semantic=vptr:quick_enforce"

// -- Routed-check throwing semantics are hard errors -------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-semantic=vptr:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=vptr:enforce' is not supported for a routed sanitizer check
// ERR-ENFORCE-SAME: noexcept_enforce
// ERR-ENFORCE-SAME: quick_enforce

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-semantic=vptr:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-OBSERVE
// ERR-OBSERVE: error: '-fsanitize-semantic=vptr:observe' is not supported for a routed sanitizer check
// ERR-OBSERVE-SAME: noexcept_observe

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:ignore %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-IGNORE
// ERR-IGNORE: error: '-fsanitize-semantic=vptr:ignore' is not supported

// -- noexcept_* requires -fcontracts-p4298 -----------------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-NOP4298
// ERR-NOP4298: error: '-fsanitize-semantic=vptr:noexcept_observe' requires '-fcontracts-p4298'

// -- -fsanitize-recover=vptr routing without p4298 is a hard error ------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-recover=vptr %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-RECOVER
// ERR-RECOVER: error: '-fsanitize-recover=vptr' routing to the contract-violation handler requires '-fcontracts-p4298'

// -- Code-path selection: continuing -> recover path, terminating -> abort ---

// noexcept_observe rides the RECOVER path -> vptr is in -fsanitize-recover=.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-semantic=vptr:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-OBSERVE
// PATH-OBSERVE: "-fsanitize-recover={{[^"]*}}vptr

// noexcept_enforce rides the NON-recover path -> vptr is NOT in
// -fsanitize-recover= (with only vptr enabled, no -fsanitize-recover= at all).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=vptr \
// RUN:   -fsanitize-semantic=vptr:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-ENFORCE
// PATH-ENFORCE-NOT: "-fsanitize-recover={{[^"]*}}vptr

// -- semantic print (real cc1 compile; -### would not run the print seam) -----

// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fsanitize-semantic-print -emit-obj -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PRINT
// PRINT: vptr: noexcept_observe

int main() { return 0; }
