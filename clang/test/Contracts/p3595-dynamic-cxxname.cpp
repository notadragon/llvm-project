// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-cxxname.json -emit-llvm -o - %s | FileCheck %s

// P3595 dynamic selection: C++ linkage with a qualified name.  The config
// names the selector "mylib::contract_semantic"; codegen must synthesize the
// enclosing namespace and Itanium-mangle the call to
// _ZN5mylib17contract_semanticEv.  This is the C++-linkage half of a
// linkage-equivalence pair: p3595-dynamic-cname.cpp targets the very same
// symbol with linkage:"C" and the mangled name given verbatim, so both
// configs must produce the same selector call.
//
// (No prior dynamic-selection test used a *qualified* C++ name -- earlier
// tests only exercised bare identifiers -- so this also covers the
// multi-component branch of the mangler.)

namespace std::contracts {
  enum class evaluation_semantic : unsigned char {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace mylib {
  std::contracts::evaluation_semantic contract_semantic();
}

void f(int x) pre(x > 0) { }

// CHECK: call{{.*}} @_ZN5mylib17contract_semanticEv
