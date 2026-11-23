// RUN: c-index-test -test-load-source local -std=c++26 -fcontracts %s | FileCheck %s

// libclang (ASTUnit) does not go through CompilerInstance::ExecuteAction; the
// contract configuration is still initialized with a DiagnosticsEngine (by
// Sema), so resolving a contract's semantic does not crash.
//
// No GCC mirror: libclang has no GCC counterpart.

int f(int x) pre(x > 0) { return x; }
int g() { return f(1); }

// CHECK: FunctionDecl=f:[[@LINE-3]]:5 (Definition)
// CHECK: FunctionDecl=g:[[@LINE-3]]:5 (Definition)
