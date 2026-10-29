// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=prove -stdlib=libc++ \
// RUN:   -x c++-header %t/h.h -o %t/h.pch
// RUN: not %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=prove -stdlib=libc++ \
// RUN:   -include-pch %t/h.pch -S -emit-llvm -o /dev/null %t/use.cpp 2>&1 \
// RUN:   | FileCheck %s
// RUN: not %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 \
// RUN:   -fcontract-evaluation-semantic=prove -stdlib=libc++ \
// RUN:   -include %t/h.h -S -emit-llvm -o /dev/null %t/use.cpp 2>&1 \
// RUN:   | FileCheck %s

// The allowed-semantics mask a P3400 label puts on a contract survives a PCH
// whole: its highest bits are now the P4405 prove and diagnose, beyond the
// eight bits the mask once had.  The configured prove moves to the analysis
// evaluation (D4405), which the label adjusts to prove and to diagnose: an
// error and a warning at the calls (a truncated mask would leave no valid
// semantic).  The same compilation without the PCH is the control.
//
// No GCC mirror: the serialized width of a Clang AST field
// (pch-contract-allowed-mask.cpp is the P4298 counterpart).

//--- h.h
#include <contracts>

struct prove_only_t {
  using assertion_control_object = prove_only_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics =
      {std::contracts::evaluation_semantic::prove};
};
constexpr prove_only_t prove_only{};

struct diagnose_only_t {
  using assertion_control_object = diagnose_only_t;
  static constexpr std::contracts::evaluation_semantic_set allowed_semantics =
      {std::contracts::evaluation_semantic::diagnose};
};
constexpr diagnose_only_t diagnose_only{};

inline void checked_one(int x) pre<prove_only>(x > 0) {}
inline void checked_two(int x) pre<diagnose_only>(x > 0) {}

//--- use.cpp
void g(int y) { checked_one(y); checked_two(y); }

// CHECK-NOT: no valid evaluation semantic
// CHECK-DAG: use.cpp:1:17: error: contract assertion cannot be proven
// CHECK-DAG: use.cpp:1:33: warning: contract assertion cannot be proven

// REQUIRES: contracts-libcxx
