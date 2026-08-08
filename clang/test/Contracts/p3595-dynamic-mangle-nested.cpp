// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-mangle-nested.json -emit-llvm -o - %s | FileCheck %s

// P3595 dynamic: a deeply nested-namespace C++ selector name (a::b::c::sel)
// mangles to the correct Itanium symbol.  Extends p3595-dynamic-cxxname.cpp,
// which used only a single-level namespace, to the multi-component mangler.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-dynamic-mangle-nested.C)

namespace std::contracts {
  enum class evaluation_semantic : unsigned char {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace a { namespace b { namespace c {
  std::contracts::evaluation_semantic sel();
}}}

void f(int x) pre(x > 0) { }

// CHECK: call{{.*}} @_ZN1a1b1c3selEv
