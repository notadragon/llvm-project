// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fcxx-exceptions -fexceptions -triple x86_64-unknown-linux-gnu -O2 -fcontract-evaluation-semantic=ignore -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:quick_enforce -emit-llvm -o - %s | FileCheck %s --implicit-check-not='@_Z1gi(i32 noundef 1)'
//
// P3100 implicit contract assertions never keep a function from being found
// nounwind from its body (refinable-body design, decision 4): going past
// one is undefined behaviour in every translation unit, whatever semantic
// each chooses for it, so what one definition shows holds for every other.
// The inline function p_div holds an implicit division check, trapping
// (quick_enforce) here, so it cannot throw: it is nounwind and its caller's
// landing pad goes (no g (1)), while d_div, which also holds a written
// contract assertion (ignored), keeps its caller's (g (2)).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-p3100.C

int g (int);

[[gnu::noinline]] inline int p_div (int a, int b) { return a / b; }
[[gnu::noinline]] inline int d_div (int a, int b)
{
  contract_assert (b != 0);
  return a / b;
}

int use_p (int a, int b) { try { return p_div (a, b); } catch (...) { return g (1); } }
int use_d (int a, int b) { try { return d_div (a, b); } catch (...) { return g (2); } }

// CHECK: define linkonce_odr {{.*}}@_Z5p_divii({{.*}}) {{.*}}[[P:#[0-9]+]]
// CHECK: call void @llvm.trap()
// CHECK-LABEL: define {{.*}}@_Z5use_dii(
// CHECK: invoke {{.*}}@_Z5d_divii(
// CHECK: {{call|invoke}} {{.*}}@_Z1gi(i32 noundef 2)
// CHECK: attributes [[P]] = { {{.*}}nounwind
