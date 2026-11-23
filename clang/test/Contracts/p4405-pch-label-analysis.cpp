// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=ignore -stdlib=libc++ \
// RUN:   -x c++-header %t/h.h -o %t/h.pch
// RUN: not %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=ignore -stdlib=libc++ \
// RUN:   -include-pch %t/h.pch -S -emit-llvm -o /dev/null %t/use.cpp 2>&1 \
// RUN:   | FileCheck %s
// RUN: not %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=ignore -stdlib=libc++ \
// RUN:   -include %t/h.h -S -emit-llvm -o /dev/null %t/use.cpp 2>&1 \
// RUN:   | FileCheck %s

// D4405: what a label's analysis_semantic facet gives the analysis
// evaluation is the label's, like its allowed set, and survives a PCH: the
// translation unit using the header resolves the contracts with it (here
// with the runtime evaluations ignore; a precondition's analysis evaluation
// is diagnosed at the call, D4405).  The same compilation without the PCH
// is the control.
//
// No GCC mirror: the serialized fields of a Clang AST node.

//--- h.h
#include <contracts>

template <std::contracts::evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr std::contracts::evaluation_semantic analysis_semantic() const {
    return S;
  }
};
constexpr analyzed<std::contracts::evaluation_semantic::prove> proven{};
constexpr analyzed<std::contracts::evaluation_semantic::diagnose> diagnosed{};
constexpr analyzed<std::contracts::evaluation_semantic::ignore> unanalyzed{};

inline int checked_one(int x) pre<proven>(x > 0) { return x; }
inline int checked_two(int x) pre<diagnosed>(x > 1) { return x; }
inline int checked_three(int x) pre<unanalyzed>(x > 2) { return x; }

//--- use.cpp
int g(int y) { return checked_one(y) + checked_two(y) + checked_three(y); }

// CHECK-DAG: use.cpp:1:23: error: contract assertion cannot be proven
// CHECK-DAG: h.h:14:{{[0-9]+}}: note: the predicate of the contract assertion
// CHECK-DAG: use.cpp:1:40: warning: contract assertion cannot be proven
// CHECK-DAG: h.h:15:{{[0-9]+}}: note: the predicate of the contract assertion
// CHECK-NOT: use.cpp:1:57:

// REQUIRES: contracts-libcxx
