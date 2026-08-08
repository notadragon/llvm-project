// P3100: -fsanitize-semantic= for the routed ThreadSanitizer (data-race) check
// -- driver diagnostics.  thread is a whole-program routed check: its allowed
// set is {assume, quick_enforce, noexcept_enforce, noexcept_observe}, and
// continue-vs-terminate is decided in the runtime report leg, so unlike the
// UBSan checks it is NOT put on the -fsanitize-recover= code-path (there is no
// per-access recover/abort variant, and -fsanitize-recover=thread is not a
// supported cc1 argument).

// -- Valid explicit requests -------------------------------------------------

// thread:noexcept_enforce is valid with -fcontracts-p4298.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=thread \
// RUN:   -fsanitize-semantic=thread:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=thread:noexcept_enforce"

// thread:quick_enforce is valid in any build (no p4298 needed) even though
// native thread has no trap mode -- quick_enforce is realized by the routing
// runtime as a silent terminate.
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:quick_enforce %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=OK-QE
// OK-QE-NOT: error:
// OK-QE: "-fsanitize-semantic=thread:quick_enforce"

// thread:noexcept_observe is valid with -fcontracts-p4298, and the driver must
// NOT put thread on the -fsanitize-recover= path (runtime-decided).
// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=thread \
// RUN:   -fsanitize-semantic=thread:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXO
// OK-NXO-NOT: error:
// OK-NXO-NOT: -fsanitize-recover={{[^ ]*}}thread
// OK-NXO: "-fsanitize-semantic=thread:noexcept_observe"

// -- Routed-check throwing semantics are hard errors -------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=thread \
// RUN:   -fsanitize-semantic=thread:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=thread:enforce' is not supported for a routed sanitizer check
// ERR-ENFORCE-SAME: noexcept_enforce
// ERR-ENFORCE-SAME: quick_enforce

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=thread \
// RUN:   -fsanitize-semantic=thread:observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-OBSERVE
// ERR-OBSERVE: error: '-fsanitize-semantic=thread:observe' is not supported for a routed sanitizer check
// ERR-OBSERVE-SAME: noexcept_observe

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:ignore %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-IGNORE
// ERR-IGNORE: error: '-fsanitize-semantic=thread:ignore' is not supported

// -- noexcept_* requires -fcontracts-p4298 -----------------------------------

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=thread -fsanitize-semantic=thread:noexcept_observe %s \
// RUN:   -### 2>&1 | FileCheck %s --check-prefix=ERR-NOP4298
// ERR-NOP4298: error: '-fsanitize-semantic=thread:noexcept_observe' requires '-fcontracts-p4298'

int main() { return 0; }
