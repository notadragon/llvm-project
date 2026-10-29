// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -S -emit-llvm \
// RUN:   -o - | FileCheck %s

// The violation arm of an explicit contract check is marked unlikely, as
// [[unlikely]] would mark it, under every semantic that checks
// (notadragon_wg21 DECISIONS-resolved.md E63 (a)): one branch with likely
// weights per check, none for a check that is not emitted.  P3100's implicit
// checks follow UBSan's handling and are not covered here.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-check-unlikely-codegen.C
// in the gnu_gcc fork.

#include <contracts>

namespace L = std::contracts::labels;
using enum std::contracts::evaluation_semantic;

constexpr L::allowed_semantics_t<noexcept_observe> nx_observe{};
constexpr L::allowed_semantics_t<noexcept_enforce> nx_enforce{};

int f_obs(int x) pre(x > 0) post(r : r > 0) {
  contract_assert(x != 3);
  return x;
}
void f_enf(int x) pre<L::always_enforce>(x > 0) {}
void f_quick(int x) pre<L::always_quick_enforce>(x > 0) {}
void f_nxo(int x) pre<nx_observe>(x > 0) {}
void f_nxe(int x) pre<nx_enforce>(x > 0) {}
void f_ign(int x) pre<L::symbolic>(x > 0) {}

// Seven checks: f_obs's three and one each for the next four; f_ign has
// none.  The success arm is the first successor, so the weights favour it.
// CHECK-COUNT-7: br i1 {{.*}}, label %{{.*}}, label %{{.*}}, !prof ![[#]]
// CHECK-NOT: !prof
// CHECK: = !{!"branch_weights", {{.*}}i32 {{[0-9]+}}, i32 1}
// CHECK-NOT: branch_weights

// REQUIRES: contracts-libcxx
