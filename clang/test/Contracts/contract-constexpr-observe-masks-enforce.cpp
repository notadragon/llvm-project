// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=enforce \
// RUN:   -fcontract-configuration-file=%S/contract-constexpr-observe-masks-enforce.json \
// RUN:   %libcxx_flags -fsyntax-only 2>&1 | FileCheck %s

// In constant evaluation, a non-terminating (observe) contract violation must
// not mask a later terminating (enforce) violation: whether an ill-formed
// program is rejected must not depend on left-to-right evaluation order.
// Namespace A resolves to observe (config file); B is enforce.  Evaluating
// A::obs(-1) (observe, fails) first must NOT swallow the subsequent B::enf(-1)
// enforce failure -- the initializer must still be ill-formed.  No Clang bug
// found (GCC F30/de95760e316 did not reproduce).
// (GCC mirror: g++.dg/contracts/cpp26/contract-constexpr-observe-masks-enforce.C.)

#include <contracts>

namespace A { constexpr int obs(int x) { contract_assert(x > 0);   return x; } }
namespace B { constexpr int enf(int x) { contract_assert(x > 100); return x; } }

// The enforce failure must be reported (not masked by the earlier observe), so
// 'bad' is not a constant expression.  The two diagnostics may be emitted in
// either order, so they are matched order-independently below.
// CHECK-DAG: error: contract failed during execution of constexpr function
// CHECK-DAG: must be initialized by a constant expression
constexpr int bad = A::obs(-1) + B::enf(-1);
