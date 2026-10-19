// P3100: -fsanitize-semantic= for the routed MemorySanitizer (Clang-only)
// use-of-uninitialized-value check -- driver diagnostics and the recover-vs-abort
// code-path selection.  Unlike ThreadSanitizer, MSan HAS recover codegen
// (__msan_warning vs __msan_warning_noreturn), so a continuing semantic drives
// -fsanitize-recover=memory and a terminating one clears it.

// -- Valid explicit requests -------------------------------------------------

// memory:noexcept_enforce is valid with -fcontracts-p4298.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=memory:noexcept_enforce"

// memory:quick_enforce is valid in any build (no p4298 needed).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:quick_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=OK-QE
// OK-QE-NOT: error:
// OK-QE: "-fsanitize-semantic=memory:quick_enforce"

// -- Code-path selection: continuing -> recover path -------------------------

// memory:noexcept_observe continues, so the driver drives -fsanitize-recover=
// memory (MSan emits the recoverable __msan_warning entry).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=RECOVER
// RECOVER: -fsanitize-recover={{[^ ]*}}memory

// -- Routed-check throwing semantics are hard errors -------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=memory:enforce' is not supported for a routed sanitizer check
// ERR-ENFORCE-SAME: noexcept_enforce
// ERR-ENFORCE-SAME: quick_enforce

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-OBSERVE
// ERR-OBSERVE: error: '-fsanitize-semantic=memory:observe' is not supported for a routed sanitizer check
// ERR-OBSERVE-SAME: noexcept_observe

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:ignore %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-IGNORE
// ERR-IGNORE: error: '-fsanitize-semantic=memory:ignore' is not supported

// -- noexcept_* requires -fcontracts-p4298 -----------------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_observe %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=ERR-NOP4298
// ERR-NOP4298: error: '-fsanitize-semantic=memory:noexcept_observe' requires '-fcontracts-p4298'

int main() { return 0; }
