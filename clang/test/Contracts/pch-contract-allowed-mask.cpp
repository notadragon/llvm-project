// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -stdlib=libc++ \
// RUN:   -x c++-header %t/h.h -o %t/h.pch
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -stdlib=libc++ \
// RUN:   -include-pch %t/h.pch -fsyntax-only -Xclang -ast-dump -Xclang -ast-dump-filter \
// RUN:   -Xclang checked_ %t/use.cpp | FileCheck %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4298 -stdlib=libc++ \
// RUN:   -include %t/h.h -fsyntax-only -Xclang -ast-dump -Xclang -ast-dump-filter \
// RUN:   -Xclang checked_ %t/use.cpp | FileCheck %s

// The allowed-semantics mask a P3400 label puts on a contract survives a PCH
// whole: its highest bits are the P4298 noexcept semantics, so a mask
// serialized narrower than the semantics it holds would lose exactly these
// labels' only allowed semantic.  The same dump without the PCH is the
// control.
//
// No GCC mirror: the serialized width of a Clang AST bit-field.

//--- h.h
#include <contracts>

struct ne_observe_only_t {
  using assertion_control_object = ne_observe_only_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics =
      {std::contracts::evaluation_semantic::noexcept_observe};
};
constexpr ne_observe_only_t ne_observe_only{};

struct ne_enforce_only_t {
  using assertion_control_object = ne_enforce_only_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics =
      {std::contracts::evaluation_semantic::noexcept_enforce};
};
constexpr ne_enforce_only_t ne_enforce_only{};

inline void checked_one(int x) pre<ne_observe_only>(x > 0) {}
inline void checked_two(int x) pre<ne_enforce_only>(x > 0) {}

//--- use.cpp
void g() {
  checked_one(1);
  checked_two(1);
}

// CHECK: FunctionDecl {{.*}} checked_one 'void (int)'
// CHECK: ContractStmt {{.*}} pre noexcept_observe{{$}}
// CHECK: FunctionDecl {{.*}} checked_two 'void (int)'
// CHECK: ContractStmt {{.*}} pre noexcept_enforce{{$}}

// REQUIRES: contracts-libcxx
