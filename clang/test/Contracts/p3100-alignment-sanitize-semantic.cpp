// P3100 Task 4.1: -fsanitize-semantic= for the routed UBSan alignment check --
// driver diagnostics, the recover-vs-abort code-path selection, and semantic
// print.  Representative of the batch of routed UBSan checks (alignment,
// object-size, nonnull-attribute, returns-nonnull-attribute, pointer-overflow);
// the logic is generic over RoutedSanitizerBits.

// -- Valid explicit requests -------------------------------------------------

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=alignment \
// RUN:   -fsanitize-semantic=alignment:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=alignment:noexcept_enforce"

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=alignment -fsanitize-semantic=alignment:quick_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=OK-QE
// OK-QE-NOT: error:
// OK-QE: "-fsanitize-semantic=alignment:quick_enforce"

// -- Routed-check throwing semantics are hard errors -------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=alignment \
// RUN:   -fsanitize-semantic=alignment:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=alignment:enforce' is not supported for a routed sanitizer check
// ERR-ENFORCE-SAME: noexcept_enforce
// ERR-ENFORCE-SAME: quick_enforce

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=alignment \
// RUN:   -fsanitize-semantic=alignment:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-OBSERVE
// ERR-OBSERVE: error: '-fsanitize-semantic=alignment:observe' is not supported for a routed sanitizer check
// ERR-OBSERVE-SAME: noexcept_observe

// -- noexcept_* requires -fcontracts-p4298 -----------------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=ERR-NOP4298
// ERR-NOP4298: error: '-fsanitize-semantic=alignment:noexcept_observe' requires '-fcontracts-p4298'

// -- Code-path selection: continuing -> recover path, terminating -> abort ---

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=alignment \
// RUN:   -fsanitize-semantic=alignment:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-OBSERVE
// PATH-OBSERVE: "-fsanitize-recover={{[^"]*}}alignment

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=alignment \
// RUN:   -fsanitize-semantic=alignment:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-ENFORCE
// PATH-ENFORCE-NOT: "-fsanitize-recover={{[^"]*}}alignment

// -- semantic print (real cc1 compile; -### would not run the print seam) -----

// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_observe \
// RUN:   -fsanitize-semantic-print -emit-obj -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PRINT
// PRINT: alignment: noexcept_observe

int main() { return 0; }
