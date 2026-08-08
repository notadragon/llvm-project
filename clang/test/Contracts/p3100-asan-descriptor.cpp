// P3100 Task 2.1 (CL2): the front end emits a per-TU weak byte
// __asan_contract_semantic conveying the resolved contract-evaluation semantic
// for the routed address check to the compiler-rt runtime.  Wire encoding
// (byte-identical to GCC): 1 = noexcept_observe, 2 = noexcept_enforce,
// 3 = quick_enforce; assume emits NO descriptor and suppresses ASan
// instrumentation for the function.  The descriptor is a weak, used global so
// it survives (Thin)LTO.

// noexcept_observe (via -fsanitize-recover=address + -fcontracts-p4298) -> 1.
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_observe %s \
// RUN:   | FileCheck %s --check-prefix=OBSERVE
// OBSERVE: @__asan_contract_semantic = weak constant i8 1
// OBSERVE: @llvm.used
// OBSERVE-SAME: __asan_contract_semantic

// noexcept_enforce (the default under p4298) -> 2.
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce %s \
// RUN:   | FileCheck %s --check-prefix=ENFORCE
// ENFORCE: @__asan_contract_semantic = weak constant i8 2

// quick_enforce (the default without p4298) -> 3.
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:quick_enforce %s \
// RUN:   | FileCheck %s --check-prefix=QUICK
// QUICK: @__asan_contract_semantic = weak constant i8 3

// assume: NO descriptor, and the function is NOT instrumented (no
// SanitizeAddress attribute -> no __asan_report* calls).
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:assume %s \
// RUN:   | FileCheck %s --check-prefix=ASSUME
// ASSUME-NOT: @__asan_contract_semantic
// ASSUME-NOT: @__asan_report
// ASSUME-NOT: {{ sanitize_address}}

// Without -fcontracts-p3100 no descriptor is emitted (stock ASan).
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fsanitize=address %s \
// RUN:   | FileCheck %s --check-prefix=STOCK
// STOCK-NOT: @__asan_contract_semantic

// P3100 Task 3.1: -fsanitize-noncontract-callbacks is the global opt-out.
// Even with -fcontracts-p3100 and a routed semantic, the opt-out suppresses
// descriptor emission entirely -- so the runtime reads stock (0) and, because
// the runtime guardrail (Task 3.2) keys off the same descriptor, the guardrail
// disengages too.
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -emit-llvm -o - \
// RUN:   -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce \
// RUN:   -fsanitize-noncontract-callbacks %s \
// RUN:   | FileCheck %s --check-prefix=OPTOUT
// OPTOUT-NOT: @__asan_contract_semantic

// ThinLTO survival: the descriptor is a weak, USED global emitted in per-TU
// CodeGen, so it is present in the ThinLTO bitcode module and marked used --
// which is exactly what protects it from whole-program elimination during the
// thin-link.  (lld/gold LTO linking is not part of this build, so survival is
// checked at the bitcode level, mirroring GCC's compile-time scan under
// -ffat-lto-objects.)
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -flto=thin -emit-llvm \
// RUN:   -o - -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=address -fsanitize-semantic=address:noexcept_enforce %s \
// RUN:   | FileCheck %s --check-prefix=THINLTO
// THINLTO: @__asan_contract_semantic = weak constant i8 2
// THINLTO: @llvm.used
// THINLTO-SAME: __asan_contract_semantic

volatile int sink;

int oob(int *p, int i) { return p[i]; }

int main() {
  int a[4] = {0, 0, 0, 0};
  sink = oob(a, 100);
  return 0;
}
