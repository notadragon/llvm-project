// The umbrella -fcontracts-p3850 must enable sanitizer routing, exactly as
// the explicit -fcontracts-p3100 does.
//
// SanitizerArgs queried -fcontracts-p3100 directly while
// CompilerInvocation::ParseLangArgs had -fcontracts-p3850 imply it, so the
// driver and cc1 disagreed: cc1 believed routing was on, the driver never
// rendered the resolved semantics and never enabled it.  The symptom was
// silent -- instrumentation byte-for-byte identical to plain -fsanitize=.
//
// GCC had the same class of bug for a different flag (-fcontracts-p3850 not
// implying -fcontracts-p4298).

// RUN: %clangxx -std=c++26 %s -fcontracts-p3850 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:assume \
// RUN:   -S -emit-llvm -o - %libcxx_flags | FileCheck %s --check-prefix=ROUTED
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:assume \
// RUN:   -S -emit-llvm -o - %libcxx_flags | FileCheck %s --check-prefix=ROUTED
// RUN: %clangxx -std=c++26 %s -fsanitize=null \
// RUN:   -S -emit-llvm -o - %libcxx_flags | FileCheck %s --check-prefix=STOCK

int deref(int *p) { return *p; }

// assume elides the check entirely, so nothing is left.
// ROUTED-NOT: __ubsan_handle

// Without routing the stock sanitizer check is still there.
// STOCK: __ubsan_handle
