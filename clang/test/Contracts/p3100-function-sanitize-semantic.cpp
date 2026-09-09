// P3100: -fsanitize-semantic= for the routed UBSan function check.
// -fsanitize=function is Clang-only (GCC has no such check), so this routed
// check has no GCC counterpart -- it exercises the same generic routing logic on
// the Clang-only bit.

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=function \
// RUN:   -fsanitize-semantic=function:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OK-NXE
// OK-NXE-NOT: error:
// OK-NXE: "-fsanitize-semantic=function:noexcept_enforce"

// RUN: not %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=function \
// RUN:   -fsanitize-semantic=function:enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=ERR-ENFORCE
// ERR-ENFORCE: error: '-fsanitize-semantic=function:enforce' is not supported for a routed sanitizer check

// -- Code-path selection: continuing -> recover, terminating -> abort --------

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=function \
// RUN:   -fsanitize-semantic=function:noexcept_observe %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-OBSERVE
// PATH-OBSERVE: "-fsanitize-recover={{[^"]*}}function

// RUN: %clang --target=x86_64-linux-gnu -c -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=function \
// RUN:   -fsanitize-semantic=function:noexcept_enforce %s -### 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PATH-ENFORCE
// PATH-ENFORCE-NOT: "-fsanitize-recover={{[^"]*}}function

// -- semantic print ----------------------------------------------------------

// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=function -fsanitize-semantic=function:noexcept_observe \
// RUN:   -fsanitize-semantic-print -emit-obj -o /dev/null %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PRINT
// PRINT: function: noexcept_observe

int main() { return 0; }
