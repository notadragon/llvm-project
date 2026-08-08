// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-cname.json -emit-llvm -o - %s | FileCheck %s

// P3595 dynamic selection: C linkage with the name given verbatim.  The
// config names the selector "_ZN5mylib17contract_semanticEv" directly (no
// mangling applied), which happens to be the very symbol that C++
// linkage + the qualified name "mylib::contract_semantic" mangles to in
// p3595-dynamic-cxxname.cpp.  Same source, same resulting call: this is the
// C-linkage half of that linkage-equivalence pair, demonstrating that
// linkage:"C" can deliberately target a mangled C++ symbol.

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
